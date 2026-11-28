#include "wm/WindowManager.hpp"

#include "fractional-scale-v1-client-protocol.h"
#include "river-input-management-v1-client-protocol.h"
#include "river-layer-shell-v1-client-protocol.h"
#include "river-libinput-config-v1-client-protocol.h"
#include "river-window-management-v1-client-protocol.h"
// river-xkb-bindings-v1.xml uses `new` as a C parameter name in `modifiers_update`
#define new river_new_modifiers
#include "river-xkb-bindings-v1-client-protocol.h"
#undef new
#include "river-xkb-config-v1-client-protocol.h"
#include "viewporter-client-protocol.h"
#include "wlr-output-management-unstable-v1-client-protocol.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <linux/input-event-codes.h>
#include <poll.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-keysyms.h>

namespace lightwm::wm {

namespace {

constexpr uint32_t kAllEdges =
    RIVER_WINDOW_V1_EDGES_TOP | RIVER_WINDOW_V1_EDGES_BOTTOM |
    RIVER_WINDOW_V1_EDGES_LEFT | RIVER_WINDOW_V1_EDGES_RIGHT;

void spawnDetached(const std::string& cmd) {
    if (cmd.empty()) return;
    pid_t pid = ::fork();
    if (pid == 0) {
        ::setsid();
        ::execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
        ::_exit(127);
    }
}

std::string resolveTerminalCommand(const QString& configured) {
    if (!configured.isEmpty() && !QStandardPaths::findExecutable(configured).isEmpty()) {
        return configured.toStdString();
    }
    const char* candidates[] = {"kitty", "foot", "alacritty", "wezterm", "ghostty", "konsole", "xterm"};
    for (const char* name : candidates) {
        if (!QStandardPaths::findExecutable(QString::fromLatin1(name)).isEmpty()) {
            return name;
        }
    }
    return configured.toStdString();
}

int createMemfdWithString(const std::string& content) {
    int fd = ::memfd_create("lightwm-keymap", MFD_CLOEXEC | MFD_ALLOW_SEALING);
    if (fd < 0) return -1;
    size_t written = 0;
    while (written < content.size()) {
        ssize_t n = ::write(fd, content.data() + written, content.size() - written);
        if (n < 0) {
            if (errno == EINTR) continue;
            ::close(fd);
            return -1;
        }
        written += static_cast<size_t>(n);
    }
    ::lseek(fd, 0, SEEK_SET);
    ::fcntl(fd, F_ADD_SEALS, F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE);
    return fd;
}

} // namespace

// ---------- Registry ----------

static void registryGlobal(void* data, wl_registry*, uint32_t name, const char* iface, uint32_t version) {
    static_cast<WindowManager*>(data)->onRegistryGlobal(name, iface, version);
}

static void registryGlobalRemove(void*, wl_registry*, uint32_t) {}

static const wl_registry_listener kRegistryListener = {
    .global = registryGlobal,
    .global_remove = registryGlobalRemove,
};

// ---------- River Window Manager ----------

static void wmUnavailable(void* data, river_window_manager_v1*) {
    std::cerr << "[lightwm-wm] river_window_manager_v1 unavailable (another WM running?)\n";
    static_cast<WindowManager*>(data)->requestQuit();
}

static void wmFinished(void* data, river_window_manager_v1*) {
    static_cast<WindowManager*>(data)->requestQuit();
}

static void wmManageStart(void* data, river_window_manager_v1*) {
    static_cast<WindowManager*>(data)->onManageStart();
}

static void wmRenderStart(void* data, river_window_manager_v1*) {
    static_cast<WindowManager*>(data)->onRenderStart();
}

static void wmSessionLocked(void* data, river_window_manager_v1*) {
    static_cast<WindowManager*>(data)->setSessionLocked(true);
}
static void wmSessionUnlocked(void* data, river_window_manager_v1*) {
    static_cast<WindowManager*>(data)->setSessionLocked(false);
}

static void wmWindow(void* data, river_window_manager_v1*, river_window_v1* win) {
    static_cast<WindowManager*>(data)->onNewWindow(win);
}

static void wmOutput(void* data, river_window_manager_v1*, river_output_v1* out) {
    static_cast<WindowManager*>(data)->onNewOutput(out);
}

static void wmSeat(void* data, river_window_manager_v1*, river_seat_v1* seat) {
    static_cast<WindowManager*>(data)->onNewSeat(seat);
}

static const river_window_manager_v1_listener kWmListener = {
    .unavailable = wmUnavailable,
    .finished = wmFinished,
    .manage_start = wmManageStart,
    .render_start = wmRenderStart,
    .session_locked = wmSessionLocked,
    .session_unlocked = wmSessionUnlocked,
    .window = wmWindow,
    .output = wmOutput,
    .seat = wmSeat,
};

// ---------- River Window ----------

struct WindowHook {
    WindowManager* wm;
    WindowState* win;
};

static void winClosed(void* data, river_window_v1*) {
    auto* hook = static_cast<WindowHook*>(data);
    hook->wm->onWindowClosed(hook->win);
    delete hook;
}

static void winDimensionsHint(void*, river_window_v1*, int32_t, int32_t, int32_t, int32_t) {}

static void winDimensions(void* data, river_window_v1*, int32_t w, int32_t h) {
    auto* win = static_cast<WindowHook*>(data)->win;
    win->currentW = w;
    win->currentH = h;
    if (!win->floatInitialized && w > 0 && h > 0) {
        win->floatRect.width = w;
        win->floatRect.height = h;
        win->floatInitialized = true;
    }
}

static void winAppId(void* data, river_window_v1*, const char* appId) {
    static_cast<WindowHook*>(data)->win->appId = appId ? appId : "";
}

static void winTitle(void* data, river_window_v1*, const char* title) {
    static_cast<WindowHook*>(data)->win->title = title ? title : "";
}

static void winParent(void* data, river_window_v1*, river_window_v1* parent) {
    if (parent) {
        static_cast<WindowHook*>(data)->win->floating = true;
    }
}

static void winDecorationHint(void*, river_window_v1*, uint32_t) {}

static void winPointerMoveRequested(void* data, river_window_v1*, river_seat_v1* seat) {
    auto* hook = static_cast<WindowHook*>(data);
    // Match seat pointer if needed; simplified to first active seat via SeatState lookup in WM
    (void)seat;
    hook->wm->beginPointerOp(nullptr, PointerOpKind::Move, hook->win->id);
}

static void winPointerResizeRequested(void* data, river_window_v1*, river_seat_v1* seat, uint32_t) {
    auto* hook = static_cast<WindowHook*>(data);
    (void)seat;
    hook->wm->beginPointerOp(nullptr, PointerOpKind::Resize, hook->win->id);
}

static void winShowWindowMenu(void*, river_window_v1*, int32_t, int32_t) {}
static void winMaximizeRequested(void*, river_window_v1*) {}
static void winUnmaximizeRequested(void*, river_window_v1*) {}

static void winFullscreenRequested(void* data, river_window_v1*, river_output_v1*) {
    auto* hook = static_cast<WindowHook*>(data);
    hook->win->fullscreen = true;
    hook->wm->scheduleManageDirty();
}

static void winExitFullscreenRequested(void* data, river_window_v1*) {
    auto* hook = static_cast<WindowHook*>(data);
    hook->win->fullscreen = false;
    hook->wm->scheduleManageDirty();
}

static void winMinimizeRequested(void*, river_window_v1*) {}
static void winUnreliablePid(void*, river_window_v1*, int32_t) {}
static void winPresentationHint(void*, river_window_v1*, uint32_t) {}
static void winIdentifier(void*, river_window_v1*, const char*) {}
static void winCaptureSessions(void*, river_window_v1*, uint32_t) {}
static void winTouchMoveRequested(void*, river_window_v1*, river_seat_v1*, int32_t) {}
static void winTouchResizeRequested(void*, river_window_v1*, river_seat_v1*, int32_t, uint32_t) {}

static const river_window_v1_listener kWindowListener = {
    .closed = winClosed,
    .dimensions_hint = winDimensionsHint,
    .dimensions = winDimensions,
    .app_id = winAppId,
    .title = winTitle,
    .parent = winParent,
    .decoration_hint = winDecorationHint,
    .pointer_move_requested = winPointerMoveRequested,
    .pointer_resize_requested = winPointerResizeRequested,
    .show_window_menu_requested = winShowWindowMenu,
    .maximize_requested = winMaximizeRequested,
    .unmaximize_requested = winUnmaximizeRequested,
    .fullscreen_requested = winFullscreenRequested,
    .exit_fullscreen_requested = winExitFullscreenRequested,
    .minimize_requested = winMinimizeRequested,
    .unreliable_pid = winUnreliablePid,
    .presentation_hint = winPresentationHint,
    .identifier = winIdentifier,
    .capture_sessions = winCaptureSessions,
    .touch_move_requested = winTouchMoveRequested,
    .touch_resize_requested = winTouchResizeRequested,
};

// ---------- River Output + Layer Shell Output ----------

struct OutputHook {
    WindowManager* wm;
    OutputState* out;
};

static void outRemoved(void* data, river_output_v1*) {
    auto* hook = static_cast<OutputHook*>(data);
    hook->wm->onOutputRemoved(hook->out);
    delete hook;
}

static void outWlOutput(void* data, river_output_v1*, uint32_t name) {
    static_cast<OutputHook*>(data)->out->wlOutputName = name;
}

static void outPosition(void* data, river_output_v1*, int32_t x, int32_t y) {
    auto* out = static_cast<OutputHook*>(data)->out;
    out->fullRect.x = x;
    out->fullRect.y = y;
    if (!out->hasNonExclusive) {
        out->usableRect.x = x;
        out->usableRect.y = y;
    }
}

static void outDimensions(void* data, river_output_v1*, int32_t w, int32_t h) {
    auto* out = static_cast<OutputHook*>(data)->out;
    out->fullRect.width = w;
    out->fullRect.height = h;
    if (!out->hasNonExclusive) {
        out->usableRect.width = w;
        out->usableRect.height = h;
    }
}

static void outCaptureSessions(void*, river_output_v1*, uint32_t) {}

static const river_output_v1_listener kOutputListener = {
    .removed = outRemoved,
    .wl_output = outWlOutput,
    .position = outPosition,
    .dimensions = outDimensions,
    .capture_sessions = outCaptureSessions,
};

static void layerOutNonExclusive(void* data, river_layer_shell_output_v1*, int32_t x, int32_t y, int32_t w, int32_t h) {
    auto* out = static_cast<OutputState*>(data);
    out->hasNonExclusive = true;
    out->usableRect = Rect{x, y, w, h};
}

static const river_layer_shell_output_v1_listener kLayerOutputListener = {
    .non_exclusive_area = layerOutNonExclusive,
};

// ---------- River Seat + Layer Shell Seat ----------

static void seatRemoved(void* data, river_seat_v1*) {
    auto* seat = static_cast<SeatState*>(data);
    seat->wm->onSeatRemoved(seat);
}

static void seatWlSeat(void* data, river_seat_v1*, uint32_t name) {
    static_cast<SeatState*>(data)->wlSeatName = name;
}

static void seatPointerEnter(void* data, river_seat_v1*, river_window_v1* win) {
    auto* seat = static_cast<SeatState*>(data);
    if (!win) {
        seat->hoveredWindowId = 0;
        return;
    }
    auto* hook = static_cast<WindowHook*>(river_window_v1_get_user_data(win));
    seat->hoveredWindowId = hook ? hook->win->id : 0;
}

static void seatPointerLeave(void* data, river_seat_v1*) {
    static_cast<SeatState*>(data)->hoveredWindowId = 0;
}

static void seatWindowInteraction(void* data, river_seat_v1*, river_window_v1* win) {
    auto* seat = static_cast<SeatState*>(data);
    if (!win) return;
    auto* hook = static_cast<WindowHook*>(river_window_v1_get_user_data(win));
    if (hook) {
        seat->hoveredWindowId = hook->win->id;
    }
}

static void seatShellSurfaceInteraction(void*, river_seat_v1*, river_shell_surface_v1*) {}

static void seatOpDelta(void* data, river_seat_v1*, int32_t dx, int32_t dy) {
    auto* seat = static_cast<SeatState*>(data);
    seat->wm->updatePointerOpDelta(seat, dx, dy);
}

static void seatOpRelease(void* data, river_seat_v1*) {
    auto* seat = static_cast<SeatState*>(data);
    seat->wm->endPointerOp(seat);
}

static void seatPointerPos(void*, river_seat_v1*, int32_t, int32_t) {}
static void seatOpDeltaTouch(void*, river_seat_v1*, int32_t, int32_t, int32_t) {}
static void seatOpReleaseTouch(void*, river_seat_v1*, int32_t) {}
static void seatOpCancelTouch(void*, river_seat_v1*, int32_t) {}

static const river_seat_v1_listener kSeatListener = {
    .removed = seatRemoved,
    .wl_seat = seatWlSeat,
    .pointer_enter = seatPointerEnter,
    .pointer_leave = seatPointerLeave,
    .window_interaction = seatWindowInteraction,
    .shell_surface_interaction = seatShellSurfaceInteraction,
    .op_delta = seatOpDelta,
    .op_release = seatOpRelease,
    .pointer_position = seatPointerPos,
    .op_delta_touch = seatOpDeltaTouch,
    .op_release_touch = seatOpReleaseTouch,
    .op_cancel_touch = seatOpCancelTouch,
};

static void layerSeatFocusExclusive(void* data, river_layer_shell_seat_v1*) {
    static_cast<SeatState*>(data)->layerExclusiveFocus = true;
}

static void layerSeatFocusNonExclusive(void* data, river_layer_shell_seat_v1*) {
    static_cast<SeatState*>(data)->layerExclusiveFocus = false;
}

static void layerSeatFocusNone(void* data, river_layer_shell_seat_v1*) {
    static_cast<SeatState*>(data)->layerExclusiveFocus = false;
}

static const river_layer_shell_seat_v1_listener kLayerSeatListener = {
    .focus_exclusive = layerSeatFocusExclusive,
    .focus_non_exclusive = layerSeatFocusNonExclusive,
    .focus_none = layerSeatFocusNone,
};

// ---------- XKB Config & Keyboards ----------

static void xkbConfigFinished(void*, river_xkb_config_v1*) {}

static void xkbConfigKeyboard(void* data, river_xkb_config_v1*, river_xkb_keyboard_v1* kbd) {
    static_cast<WindowManager*>(data)->onNewXkbKeyboard(kbd);
}

static const river_xkb_config_v1_listener kXkbConfigListener = {
    .finished = xkbConfigFinished,
    .xkb_keyboard = xkbConfigKeyboard,
};

struct KeymapHook {
    WindowManager* wm;
    river_xkb_keymap_v1* km;
};

static void keymapSuccess(void* data, river_xkb_keymap_v1*) {
    auto* hook = static_cast<KeymapHook*>(data);
    hook->wm->onKeymapCreated(hook->km, true, nullptr);
    delete hook;
}

static void keymapFailure(void* data, river_xkb_keymap_v1*, const char* msg) {
    auto* hook = static_cast<KeymapHook*>(data);
    hook->wm->onKeymapCreated(hook->km, false, msg);
    delete hook;
}

static const river_xkb_keymap_v1_listener kXkbKeymapListener = {
    .success = keymapSuccess,
    .failure = keymapFailure,
};

static void kbdRemoved(void* data, river_xkb_keyboard_v1*) {
    auto* kbd = static_cast<XkbKeyboardState*>(data);
    kbd->wm->onXkbKeyboardRemoved(kbd);
}
static void kbdInputDevice(void*, river_xkb_keyboard_v1*, river_input_device_v1*) {}

static void kbdLayout(void* data, river_xkb_keyboard_v1*, uint32_t idx, const char* name) {
    auto* kbd = static_cast<XkbKeyboardState*>(data);
    kbd->currentLayoutIndex = idx;
    kbd->currentLayoutName = name ? name : "us";
    kbd->wm->updateKeyboardLayoutDisplay(kbd->currentLayoutName);
}

static void kbdCapsEnabled(void*, river_xkb_keyboard_v1*) {}
static void kbdCapsDisabled(void*, river_xkb_keyboard_v1*) {}
static void kbdNumEnabled(void*, river_xkb_keyboard_v1*) {}
static void kbdNumDisabled(void*, river_xkb_keyboard_v1*) {}
static void kbdDone(void*, river_xkb_keyboard_v1*) {}
static void kbdScrollEnabled(void*, river_xkb_keyboard_v1*) {}
static void kbdScrollDisabled(void*, river_xkb_keyboard_v1*) {}

static const river_xkb_keyboard_v1_listener kXkbKeyboardListener = {
    .removed = kbdRemoved,
    .input_device = kbdInputDevice,
    .layout = kbdLayout,
    .capslock_enabled = kbdCapsEnabled,
    .capslock_disabled = kbdCapsDisabled,
    .numlock_enabled = kbdNumEnabled,
    .numlock_disabled = kbdNumDisabled,
    .done = kbdDone,
    .scrolllock_enabled = kbdScrollEnabled,
    .scrolllock_disabled = kbdScrollDisabled,
};

// ---------- Libinput Config ----------

static void libinputFinished(void*, river_libinput_config_v1*) {}

static void libinputDevice(void* data, river_libinput_config_v1*, river_libinput_device_v1* dev) {
    static_cast<WindowManager*>(data)->onNewLibinputDevice(dev);
}

static const river_libinput_config_v1_listener kLibinputConfigListener = {
    .finished = libinputFinished,
    .libinput_device = libinputDevice,
};

static void libdevRemoved(void* data, river_libinput_device_v1*) {
    auto* dev = static_cast<LibinputDeviceState*>(data);
    dev->wm->onLibinputDeviceRemoved(dev);
}
static void libdevInputDevice(void*, river_libinput_device_v1*, river_input_device_v1*) {}
static void libdevTapSupport(void* data, river_libinput_device_v1*, int32_t fingers) {
    static_cast<LibinputDeviceState*>(data)->tapFingerCount = fingers;
}
static void libdevTapCurrent(void* data, river_libinput_device_v1*, uint32_t state) {
    static_cast<LibinputDeviceState*>(data)->tapCurrent = state;
}
static void libdevNaturalScrollSupport(void* data, river_libinput_device_v1*, int32_t supported) {
    static_cast<LibinputDeviceState*>(data)->naturalScrollSupported = supported;
}
static void libdevNaturalScrollCurrent(void* data, river_libinput_device_v1*, uint32_t state) {
    static_cast<LibinputDeviceState*>(data)->naturalScrollCurrent = state;
}
static void libdevAccelSpeedDefault(void* data, river_libinput_device_v1*, wl_array* speed) {
    if (speed && speed->size == sizeof(double)) {
        static_cast<LibinputDeviceState*>(data)->accelSpeedSupported = true;
    }
}
static void libdevAccelSpeedCurrent(void* data, river_libinput_device_v1*, wl_array* speed) {
    if (speed && speed->size == sizeof(double)) {
        auto* dev = static_cast<LibinputDeviceState*>(data);
        dev->accelSpeedCurrent = *static_cast<const double*>(speed->data);
        dev->hasAccelSpeedCurrent = true;
        dev->accelSpeedSupported = true;
    }
}
static void libdevDone(void* data, river_libinput_device_v1*) {
    auto* dev = static_cast<LibinputDeviceState*>(data);
    if (!dev->configured) {
        dev->configured = true;
        dev->wm->applyLibinputConfig(dev);
    }
}

static void libdevNoopU(void*, river_libinput_device_v1*, uint32_t) {}
static void libdevNoopI(void*, river_libinput_device_v1*, int32_t) {}
static void libdevNoopA(void*, river_libinput_device_v1*, wl_array*) {}

static const river_libinput_device_v1_listener kLibinputDeviceListener = {
    .removed = libdevRemoved,
    .input_device = libdevInputDevice,
    .send_events_support = libdevNoopU,
    .send_events_default = libdevNoopU,
    .send_events_current = libdevNoopU,
    .tap_support = libdevTapSupport,
    .tap_default = libdevNoopU,
    .tap_current = libdevTapCurrent,
    .tap_button_map_default = libdevNoopU,
    .tap_button_map_current = libdevNoopU,
    .drag_default = libdevNoopU,
    .drag_current = libdevNoopU,
    .drag_lock_default = libdevNoopU,
    .drag_lock_current = libdevNoopU,
    .three_finger_drag_support = libdevNoopI,
    .three_finger_drag_default = libdevNoopU,
    .three_finger_drag_current = libdevNoopU,
    .calibration_matrix_support = libdevNoopI,
    .calibration_matrix_default = libdevNoopA,
    .calibration_matrix_current = libdevNoopA,
    .accel_profiles_support = libdevNoopU,
    .accel_profile_default = libdevNoopU,
    .accel_profile_current = libdevNoopU,
    .accel_speed_default = libdevAccelSpeedDefault,
    .accel_speed_current = libdevAccelSpeedCurrent,
    .natural_scroll_support = libdevNaturalScrollSupport,
    .natural_scroll_default = libdevNoopU,
    .natural_scroll_current = libdevNaturalScrollCurrent,
    .left_handed_support = libdevNoopI,
    .left_handed_default = libdevNoopU,
    .left_handed_current = libdevNoopU,
    .click_method_support = libdevNoopU,
    .click_method_default = libdevNoopU,
    .click_method_current = libdevNoopU,
    .clickfinger_button_map_default = libdevNoopU,
    .clickfinger_button_map_current = libdevNoopU,
    .middle_emulation_support = libdevNoopI,
    .middle_emulation_default = libdevNoopU,
    .middle_emulation_current = libdevNoopU,
    .scroll_method_support = libdevNoopU,
    .scroll_method_default = libdevNoopU,
    .scroll_method_current = libdevNoopU,
    .scroll_button_default = libdevNoopU,
    .scroll_button_current = libdevNoopU,
    .scroll_button_lock_default = libdevNoopU,
    .scroll_button_lock_current = libdevNoopU,
    .dwt_support = libdevNoopI,
    .dwt_default = libdevNoopU,
    .dwt_current = libdevNoopU,
    .dwtp_support = libdevNoopI,
    .dwtp_default = libdevNoopU,
    .dwtp_current = libdevNoopU,
    .rotation_support = libdevNoopI,
    .rotation_default = libdevNoopU,
    .rotation_current = libdevNoopU,
    .done = libdevDone,
};

static void libResultSuccess(void*, river_libinput_result_v1* res) {
    river_libinput_result_v1_destroy(res);
}
static void libResultUnsupported(void*, river_libinput_result_v1* res) {
    river_libinput_result_v1_destroy(res);
}
static void libResultInvalid(void*, river_libinput_result_v1* res) {
    river_libinput_result_v1_destroy(res);
}
static const river_libinput_result_v1_listener kLibinputResultListener = {
    .success = libResultSuccess,
    .unsupported = libResultUnsupported,
    .invalid = libResultInvalid,
};

// ---------- XKB & Pointer Bindings ----------

static void xkbBindingPressedDispatch(void* data, river_xkb_binding_v1*);
static void xkbBindingReleased(void*, river_xkb_binding_v1*) {}
static void xkbBindingStopRepeat(void*, river_xkb_binding_v1*) {}

static const river_xkb_binding_v1_listener kXkbBindingListener = {
    .pressed = xkbBindingPressedDispatch,
    .released = xkbBindingReleased,
    .stop_repeat = xkbBindingStopRepeat,
};

static void ptrBindingPressed(void* data, river_pointer_binding_v1*) {
    auto* p = static_cast<WindowManager::PointerBindingData*>(data);
    p->wm->beginPointerOp(p->seat, p->op, 0);
}

static void ptrBindingReleased(void* data, river_pointer_binding_v1*) {
    auto* p = static_cast<WindowManager::PointerBindingData*>(data);
    p->wm->endPointerOp(p->seat);
}

static const river_pointer_binding_v1_listener kPtrBindingListener = {
    .pressed = ptrBindingPressed,
    .released = ptrBindingReleased,
};

// ---------- Fractional Scale for Corner Overlays ----------

static void cornerPreferredScale(void* data, wp_fractional_scale_v1*, uint32_t scale) {
    auto* win = static_cast<WindowState*>(data);
    if (win && scale > 0 && win->cornerScale120 != scale) {
        win->cornerScale120 = scale;
        if (win->wm) {
            win->wm->scheduleManageDirty();
        }
    }
}

static const wp_fractional_scale_v1_listener kCornerScaleListener = {
    .preferred_scale = cornerPreferredScale,
};

// ---------- WindowManager Implementation ----------

WindowManager::WindowManager() = default;

WindowManager::~WindowManager() {
    for (auto& win : m_windows) {
        if (win->cornerFracScale) wp_fractional_scale_v1_destroy(win->cornerFracScale);
        for (int i = 0; i < 4; ++i) {
            if (win->cornerViewport[i]) wp_viewport_destroy(win->cornerViewport[i]);
            if (win->cornerDec[i]) river_decoration_v1_destroy(win->cornerDec[i]);
            if (win->cornerSurf[i]) wl_surface_destroy(win->cornerSurf[i]);
        }
    }
    m_corners.destroyBuffers();
    m_outputScaleMgr.reset();
    if (m_ipc) m_ipc->stop();
    if (m_activeKeymap) river_xkb_keymap_v1_destroy(m_activeKeymap);
    if (m_pendingKeymap) river_xkb_keymap_v1_destroy(m_pendingKeymap);
    if (m_fractionalScaleMgr) wp_fractional_scale_manager_v1_destroy(m_fractionalScaleMgr);
    if (m_viewporter) wp_viewporter_destroy(m_viewporter);
    if (m_layerShell) river_layer_shell_v1_destroy(m_layerShell);
    if (m_shm) wl_shm_destroy(m_shm);
    if (m_compositor) wl_compositor_destroy(m_compositor);
    if (m_display) wl_display_disconnect(m_display);
    if (m_xkbCtx) xkb_context_unref(m_xkbCtx);
}

bool WindowManager::init(const std::string& ipcSocketPath) {
    m_config = ConfigStore::load();
    m_palette = paletteForName(m_config.themeName);
    m_xkbCtx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    m_outputScaleMgr = std::make_unique<OutputScaleManager>([this]() {
        broadcastIpcState();
    });

    m_display = wl_display_connect(nullptr);
    if (!m_display) {
        std::cerr << "[lightwm-wm] Failed to connect to Wayland display\n";
        return false;
    }

    m_registry = wl_display_get_registry(m_display);
    wl_registry_add_listener(m_registry, &kRegistryListener, this);
    wl_display_roundtrip(m_display);

    if (!m_wm) {
        std::cerr << "[lightwm-wm] river_window_manager_v1 global not found\n";
        return false;
    }

    m_outputScaleMgr->setDesiredScales(m_config.outputScales);

    m_ipc = std::make_unique<IpcServer>([this](const QJsonObject& cmd) {
        onActionIpcCommand(cmd);
    });
    if (!m_ipc->start(ipcSocketPath)) {
        std::cerr << "[lightwm-wm] Warning: failed to start IPC server at " << ipcSocketPath << "\n";
    }

    m_gestures = std::make_unique<TouchpadGestures>([this](int dir) {
        if (m_sessionLocked) return;
        if (auto* out = primaryOutput()) {
            const int target = std::clamp(out->activeWorkspace + dir, 1, m_config.workspaceCount);
            if (target != out->activeWorkspace) {
                onActionSwitchWorkspace(target);
            }
        }
    });
    m_gestures->init();

    if (m_xkbConfig) {
        rebuildAndPushXkbKeymap();
    }

    m_corners.init(m_compositor, m_shm);
    m_corners.rebuild(m_config.cornerRadius, m_config.borderWidth, m_palette);

    return true;
}

void WindowManager::onRegistryGlobal(uint32_t name, const char* iface, uint32_t version) {
    const std::string nameStr(iface);
    if (nameStr == wl_compositor_interface.name) {
        m_compositor = static_cast<wl_compositor*>(
            wl_registry_bind(m_registry, name, &wl_compositor_interface, std::min(version, 4u)));
    } else if (nameStr == wl_shm_interface.name) {
        m_shm = static_cast<wl_shm*>(
            wl_registry_bind(m_registry, name, &wl_shm_interface, 1u));
    } else if (nameStr == wp_viewporter_interface.name) {
        m_viewporter = static_cast<wp_viewporter*>(
            wl_registry_bind(m_registry, name, &wp_viewporter_interface, 1u));
    } else if (nameStr == wp_fractional_scale_manager_v1_interface.name) {
        m_fractionalScaleMgr = static_cast<wp_fractional_scale_manager_v1*>(
            wl_registry_bind(m_registry, name, &wp_fractional_scale_manager_v1_interface, 1u));
    } else if (nameStr == zwlr_output_manager_v1_interface.name && m_outputScaleMgr) {
        m_outputScaleMgr->bind(m_registry, name, version);
    } else if (nameStr == river_window_manager_v1_interface.name) {
        m_wm = static_cast<river_window_manager_v1*>(
            wl_registry_bind(m_registry, name, &river_window_manager_v1_interface, std::min(version, 6u)));
        river_window_manager_v1_add_listener(m_wm, &kWmListener, this);
    } else if (nameStr == river_layer_shell_v1_interface.name) {
        m_layerShell = static_cast<river_layer_shell_v1*>(
            wl_registry_bind(m_registry, name, &river_layer_shell_v1_interface, std::min(version, 1u)));
    } else if (nameStr == river_xkb_bindings_v1_interface.name) {
        m_xkbBindings = static_cast<river_xkb_bindings_v1*>(
            wl_registry_bind(m_registry, name, &river_xkb_bindings_v1_interface, std::min(version, 3u)));
    } else if (nameStr == river_xkb_config_v1_interface.name) {
        m_xkbConfig = static_cast<river_xkb_config_v1*>(
            wl_registry_bind(m_registry, name, &river_xkb_config_v1_interface, std::min(version, 3u)));
        river_xkb_config_v1_add_listener(m_xkbConfig, &kXkbConfigListener, this);
    } else if (nameStr == river_input_manager_v1_interface.name) {
        m_inputMgr = static_cast<river_input_manager_v1*>(
            wl_registry_bind(m_registry, name, &river_input_manager_v1_interface, std::min(version, 2u)));
    } else if (nameStr == river_libinput_config_v1_interface.name) {
        m_libinputCfg = static_cast<river_libinput_config_v1*>(
            wl_registry_bind(m_registry, name, &river_libinput_config_v1_interface, std::min(version, 2u)));
        river_libinput_config_v1_add_listener(m_libinputCfg, &kLibinputConfigListener, this);
    }
}

int WindowManager::run() {
    const int wlFd = wl_display_get_fd(m_display);

    while (m_running) {
        while (wl_display_prepare_read(m_display) != 0) {
            wl_display_dispatch_pending(m_display);
        }
        wl_display_flush(m_display);

        std::vector<pollfd> fds;
        fds.push_back(pollfd{wlFd, POLLIN, 0});
        const int gestureFd = m_gestures ? m_gestures->fd() : -1;
        if (gestureFd >= 0) {
            fds.push_back(pollfd{gestureFd, POLLIN, 0});
        }
        if (m_ipc) {
            m_ipc->appendPollFds(fds);
        }

        const int timeoutMs = nextAnimationPollTimeoutMs();
        const int ret = ::poll(fds.data(), static_cast<nfds_t>(fds.size()), timeoutMs);
        if (ret < 0) {
            wl_display_cancel_read(m_display);
            if (errno == EINTR) continue;
            break;
        }

        if (fds[0].revents & POLLIN) {
            wl_display_read_events(m_display);
            if (wl_display_dispatch_pending(m_display) < 0) {
                break;
            }
        } else {
            wl_display_cancel_read(m_display);
        }

        if (fds[0].revents & (POLLERR | POLLHUP)) {
            break;
        }

        if (gestureFd >= 0) {
            for (const auto& p : fds) {
                if (p.fd == gestureFd && (p.revents & POLLIN)) {
                    m_gestures->processEvents();
                    break;
                }
            }
        }

        if (m_ipc) {
            m_ipc->handlePollEvents(fds);
        }

        if (hasActiveAnimations()) {
            stepAnimations();
        }
    }

    return 0;
}

void WindowManager::onNewWindow(river_window_v1* wlWin) {
    auto win = std::make_unique<WindowState>();
    win->wm = this;
    win->id = m_nextId++;
    win->wl = wlWin;
    win->node = river_window_v1_get_node(wlWin);

    if (auto* out = primaryOutput()) {
        win->outputId = out->id;
        win->workspace = out->activeWorkspace;
    }

    auto* hook = new WindowHook{this, win.get()};
    river_window_v1_add_listener(wlWin, &kWindowListener, hook);
    m_focusedWindowId = win->id;
    m_windows.push_back(std::move(win));
}

void WindowManager::onNewOutput(river_output_v1* wlOut) {
    auto out = std::make_unique<OutputState>();
    out->id = m_nextId++;
    out->wl = wlOut;
    if (m_layerShell) {
        out->layerOutput = river_layer_shell_v1_get_output(m_layerShell, wlOut);
        river_layer_shell_output_v1_add_listener(out->layerOutput, &kLayerOutputListener, out.get());
    }
    auto* hook = new OutputHook{this, out.get()};
    river_output_v1_add_listener(wlOut, &kOutputListener, hook);
    m_outputs.push_back(std::move(out));
}

void WindowManager::onNewSeat(river_seat_v1* wlSeat) {
    auto seat = std::make_unique<SeatState>();
    seat->wm = this;
    seat->wl = wlSeat;
    if (m_layerShell) {
        seat->layerSeat = river_layer_shell_v1_get_seat(m_layerShell, wlSeat);
        river_layer_shell_seat_v1_add_listener(seat->layerSeat, &kLayerSeatListener, seat.get());
    }
    river_seat_v1_add_listener(wlSeat, &kSeatListener, seat.get());
    setupSeatBindings(seat.get());
    m_seats.push_back(std::move(seat));
}

void WindowManager::onNewXkbKeyboard(river_xkb_keyboard_v1* kbd) {
    auto state = std::make_unique<XkbKeyboardState>();
    state->wm = this;
    state->wl = kbd;
    river_xkb_keyboard_v1_add_listener(kbd, &kXkbKeyboardListener, state.get());
    if (m_activeKeymap && m_keymapReady) {
        river_xkb_keyboard_v1_set_keymap(kbd, m_activeKeymap);
    }
    m_keyboards.push_back(std::move(state));
}

void WindowManager::onNewLibinputDevice(river_libinput_device_v1* dev) {
    auto state = std::make_unique<LibinputDeviceState>();
    state->wm = this;
    state->wl = dev;
    river_libinput_device_v1_add_listener(dev, &kLibinputDeviceListener, state.get());
    m_libinputDevices.push_back(std::move(state));
}

void WindowManager::applyLibinputConfig(LibinputDeviceState* dev) {
    if (!dev || !dev->wl || dev->removed) return;
    if (dev->tapFingerCount > 0) {
        const uint32_t wantTap = m_config.tapToClick
            ? RIVER_LIBINPUT_DEVICE_V1_TAP_STATE_ENABLED
            : RIVER_LIBINPUT_DEVICE_V1_TAP_STATE_DISABLED;
        if (dev->tapCurrent != wantTap) {
            auto* res = river_libinput_device_v1_set_tap(dev->wl, wantTap);
            river_libinput_result_v1_add_listener(res, &kLibinputResultListener, nullptr);
        }
    }
    if (dev->naturalScrollSupported) {
        const uint32_t wantNatural = m_config.naturalScroll
            ? RIVER_LIBINPUT_DEVICE_V1_NATURAL_SCROLL_STATE_ENABLED
            : RIVER_LIBINPUT_DEVICE_V1_NATURAL_SCROLL_STATE_DISABLED;
        if (dev->naturalScrollCurrent != wantNatural) {
            auto* res = river_libinput_device_v1_set_natural_scroll(dev->wl, wantNatural);
            river_libinput_result_v1_add_listener(res, &kLibinputResultListener, nullptr);
        }
    }
    if (dev->accelSpeedSupported) {
        const double wantSpeed = std::clamp(m_config.pointerSpeed, -1.0, 1.0);
        if (!dev->hasAccelSpeedCurrent || std::abs(dev->accelSpeedCurrent - wantSpeed) > 0.01) {
            wl_array arr;
            wl_array_init(&arr);
            if (auto* p = static_cast<double*>(wl_array_add(&arr, sizeof(double)))) {
                *p = wantSpeed;
                auto* res = river_libinput_device_v1_set_accel_speed(dev->wl, &arr);
                river_libinput_result_v1_add_listener(res, &kLibinputResultListener, nullptr);
            }
            wl_array_release(&arr);
        }
    }
    dev->configured = true;
}

void WindowManager::onLibinputDeviceRemoved(LibinputDeviceState* dev) {
    if (!dev) return;
    dev->removed = true;
    if (dev->wl) {
        river_libinput_device_v1_destroy(dev->wl);
        dev->wl = nullptr;
    }
    m_libinputDevices.erase(
        std::remove_if(m_libinputDevices.begin(), m_libinputDevices.end(),
                       [dev](const auto& ptr) { return ptr.get() == dev; }),
        m_libinputDevices.end());
}

void WindowManager::onXkbKeyboardRemoved(XkbKeyboardState* kbd) {
    if (!kbd) return;
    if (kbd->wl) {
        river_xkb_keyboard_v1_destroy(kbd->wl);
        kbd->wl = nullptr;
    }
    m_keyboards.erase(
        std::remove_if(m_keyboards.begin(), m_keyboards.end(),
                       [kbd](const auto& ptr) { return ptr.get() == kbd; }),
        m_keyboards.end());
}

void WindowManager::onWindowClosed(WindowState* win) {
    win->closed = true;
    if (m_focusedWindowId == win->id) {
        m_focusedWindowId = 0;
    }
    if (win->cornerFracScale) {
        wp_fractional_scale_v1_destroy(win->cornerFracScale);
        win->cornerFracScale = nullptr;
    }
    for (int i = 0; i < 4; ++i) {
        if (win->cornerViewport[i]) {
            wp_viewport_destroy(win->cornerViewport[i]);
            win->cornerViewport[i] = nullptr;
        }
        if (win->cornerDec[i]) {
            river_decoration_v1_destroy(win->cornerDec[i]);
            win->cornerDec[i] = nullptr;
        }
        if (win->cornerSurf[i]) {
            wl_surface_destroy(win->cornerSurf[i]);
            win->cornerSurf[i] = nullptr;
        }
    }
    if (win->node) {
        river_node_v1_destroy(win->node);
        win->node = nullptr;
    }
    river_window_v1_destroy(win->wl);
    win->wl = nullptr;

    m_windows.erase(
        std::remove_if(m_windows.begin(), m_windows.end(),
                       [win](const auto& ptr) { return ptr.get() == win; }),
        m_windows.end());

    ensureValidFocusOnOutput(primaryOutput());
    scheduleManageDirty();
}

void WindowManager::onOutputRemoved(OutputState* out) {
    if (out->layerOutput) river_layer_shell_output_v1_destroy(out->layerOutput);
    river_output_v1_destroy(out->wl);
    m_outputs.erase(
        std::remove_if(m_outputs.begin(), m_outputs.end(),
                       [out](const auto& ptr) { return ptr.get() == out; }),
        m_outputs.end());
    scheduleManageDirty();
}

void WindowManager::onSeatRemoved(SeatState* seat) {
    if (seat->layerSeat) river_layer_shell_seat_v1_destroy(seat->layerSeat);
    river_seat_v1_destroy(seat->wl);
    m_seats.erase(
        std::remove_if(m_seats.begin(), m_seats.end(),
                       [seat](const auto& ptr) { return ptr.get() == seat; }),
        m_seats.end());
}

void WindowManager::setupSeatBindings(SeatState* seat) {
    if (!m_xkbBindings || seat->bindingsCreated) return;

    const uint32_t modSuper = RIVER_SEAT_V1_MODIFIERS_MOD4;
    const uint32_t modSuperShift = RIVER_SEAT_V1_MODIFIERS_MOD4 | RIVER_SEAT_V1_MODIFIERS_SHIFT;

    auto addKey = [&](uint32_t keysym, uint32_t mods, BindingAction action) {
        auto* binding = river_xkb_bindings_v1_get_xkb_binding(m_xkbBindings, seat->wl, keysym, mods);
        auto bd = std::make_unique<XkbBindingData>();
        bd->wm = this;
        bd->seat = seat;
        bd->action = action;
        bd->wl = binding;
        river_xkb_binding_v1_add_listener(binding, &kXkbBindingListener, bd.get());
        seat->xkbBindings.push_back(binding);
        m_xkbBindingData.push_back(std::move(bd));
    };

    addKey(XKB_KEY_q, modSuper, BindingAction::SpawnTerminal);
    addKey(XKB_KEY_Q, modSuper, BindingAction::SpawnTerminal);
    addKey(XKB_KEY_c, modSuper, BindingAction::CloseFocused);
    addKey(XKB_KEY_C, modSuper, BindingAction::CloseFocused);
    addKey(XKB_KEY_v, modSuper, BindingAction::ToggleFloat);
    addKey(XKB_KEY_V, modSuper, BindingAction::ToggleFloat);
    addKey(XKB_KEY_f, modSuper, BindingAction::ToggleFullscreen);
    addKey(XKB_KEY_F, modSuper, BindingAction::ToggleFullscreen);
    addKey(XKB_KEY_h, modSuper, BindingAction::FocusLeft);
    addKey(XKB_KEY_H, modSuper, BindingAction::FocusLeft);
    addKey(XKB_KEY_j, modSuper, BindingAction::FocusDown);
    addKey(XKB_KEY_J, modSuper, BindingAction::FocusDown);
    addKey(XKB_KEY_k, modSuper, BindingAction::FocusUp);
    addKey(XKB_KEY_K, modSuper, BindingAction::FocusUp);
    addKey(XKB_KEY_l, modSuper, BindingAction::FocusRight);
    addKey(XKB_KEY_L, modSuper, BindingAction::FocusRight);
    addKey(XKB_KEY_r, modSuper, BindingAction::ToggleLauncher);
    addKey(XKB_KEY_R, modSuper, BindingAction::ToggleLauncher);
    addKey(XKB_KEY_l, modSuperShift, BindingAction::LockScreen);
    addKey(XKB_KEY_L, modSuperShift, BindingAction::LockScreen);
    addKey(XKB_KEY_Print, 0, BindingAction::Screenshot);
    addKey(XKB_KEY_XF86AudioRaiseVolume, 0, BindingAction::AudioRaise);
    addKey(XKB_KEY_XF86AudioLowerVolume, 0, BindingAction::AudioLower);
    addKey(XKB_KEY_XF86AudioMute, 0, BindingAction::AudioMute);
    addKey(XKB_KEY_XF86MonBrightnessUp, 0, BindingAction::BrightnessUp);
    addKey(XKB_KEY_XF86MonBrightnessDown, 0, BindingAction::BrightnessDown);

    const uint32_t numKeys[9] = {
        XKB_KEY_1, XKB_KEY_2, XKB_KEY_3, XKB_KEY_4, XKB_KEY_5,
        XKB_KEY_6, XKB_KEY_7, XKB_KEY_8, XKB_KEY_9
    };
    const uint32_t shiftSyms[9] = {
        XKB_KEY_exclam, XKB_KEY_at, XKB_KEY_numbersign, XKB_KEY_dollar, XKB_KEY_percent,
        XKB_KEY_asciicircum, XKB_KEY_ampersand, XKB_KEY_asterisk, XKB_KEY_parenleft
    };
    for (int i = 0; i < 9; ++i) {
        addKey(numKeys[i], modSuper, static_cast<BindingAction>(static_cast<int>(BindingAction::SwitchWorkspace1) + i));
        addKey(numKeys[i], modSuperShift, static_cast<BindingAction>(static_cast<int>(BindingAction::MoveWorkspace1) + i));
        addKey(shiftSyms[i], modSuperShift, static_cast<BindingAction>(static_cast<int>(BindingAction::MoveWorkspace1) + i));
    }

    auto addPtr = [&](uint32_t btn, uint32_t mods, PointerOpKind op) {
        auto* pb = river_seat_v1_get_pointer_binding(seat->wl, btn, mods);
        auto pd = std::make_unique<PointerBindingData>();
        pd->wm = this;
        pd->seat = seat;
        pd->op = op;
        pd->wl = pb;
        river_pointer_binding_v1_add_listener(pb, &kPtrBindingListener, pd.get());
        seat->pointerBindings.push_back(pb);
        m_pointerBindingData.push_back(std::move(pd));
    };

    addPtr(BTN_LEFT, modSuper, PointerOpKind::Move);
    addPtr(BTN_RIGHT, modSuper, PointerOpKind::Resize);

    seat->bindingsCreated = true;
    seat->bindingsDirty = true;
}

static void xkbBindingPressedDispatch(void* data, river_xkb_binding_v1*) {
    auto* b = static_cast<WindowManager::XkbBindingData*>(data);
    using A = WindowManager::BindingAction;
    const int idx = static_cast<int>(b->action);
    if (idx >= static_cast<int>(A::SwitchWorkspace1) && idx <= static_cast<int>(A::SwitchWorkspace9)) {
        b->wm->onActionSwitchWorkspace(1 + (idx - static_cast<int>(A::SwitchWorkspace1)));
        return;
    }
    if (idx >= static_cast<int>(A::MoveWorkspace1) && idx <= static_cast<int>(A::MoveWorkspace9)) {
        b->wm->onActionMoveWindowToWorkspace(1 + (idx - static_cast<int>(A::MoveWorkspace1)));
        return;
    }

    switch (b->action) {
        case A::SpawnTerminal: b->wm->onActionIpcCommand({{"cmd", "spawn_terminal"}}); break;
        case A::CloseFocused: b->wm->onActionCloseFocused(); break;
        case A::ToggleFloat: b->wm->onActionToggleFloatFocused(); break;
        case A::ToggleFullscreen: b->wm->onActionToggleFullscreenFocused(); break;
        case A::FocusLeft: b->wm->onActionFocusDirection(WindowManager::FocusDirection::Left); break;
        case A::FocusDown: b->wm->onActionFocusDirection(WindowManager::FocusDirection::Down); break;
        case A::FocusUp: b->wm->onActionFocusDirection(WindowManager::FocusDirection::Up); break;
        case A::FocusRight: b->wm->onActionFocusDirection(WindowManager::FocusDirection::Right); break;
        case A::ToggleLauncher: b->wm->onActionIpcCommand({{"event", "toggle_launcher"}}); break;
        case A::LockScreen: b->wm->onActionIpcCommand({{"event", "lock_screen"}}); break;
        case A::Screenshot: b->wm->onActionIpcCommand({{"event", "screenshot"}}); break;
        case A::AudioRaise: b->wm->onActionIpcCommand({{"event", "audio_step"}, {"delta", 5}}); break;
        case A::AudioLower: b->wm->onActionIpcCommand({{"event", "audio_step"}, {"delta", -5}}); break;
        case A::AudioMute: b->wm->onActionIpcCommand({{"event", "audio_mute_toggle"}}); break;
        case A::BrightnessUp: b->wm->onActionIpcCommand({{"event", "brightness_step"}, {"delta", 5}}); break;
        case A::BrightnessDown: b->wm->onActionIpcCommand({{"event", "brightness_step"}, {"delta", -5}}); break;
        default: break;
    }
}

void WindowManager::onManageStart() {
    m_inManageSequence = true;
    m_dirtyQueued = false;

    if (m_gestures && !m_gestures->hasGestureDevice() && m_gestureRetryCount < 5) {
        ++m_gestureRetryCount;
        m_gestures->init();
    }

    // Window management state changes (like closing finished animations) must happen in manage_start
    for (auto& win : m_windows) {
        if (win->pendingClose) {
            win->pendingClose = false;
            river_window_v1_close(win->wl);
        }
    }

    for (auto& seat : m_seats) {
        if (seat->bindingsDirty) {
            for (auto* kb : seat->xkbBindings) {
                river_xkb_binding_v1_set_layout_override(kb, 0);
                river_xkb_binding_v1_enable(kb);
            }
            for (auto* pb : seat->pointerBindings) river_pointer_binding_v1_enable(pb);
            seat->bindingsDirty = false;
        }
        if (seat->pendingStartOp != PointerOpKind::None) {
            river_seat_v1_op_start_pointer(seat->wl);
            seat->activeOp = seat->pendingStartOp;
            seat->pendingStartOp = PointerOpKind::None;
        }
        if (seat->pendingEndOp) {
            river_seat_v1_op_end(seat->wl);
            seat->activeOp = PointerOpKind::None;
            seat->pendingEndOp = false;
            seat->opWindowId = 0;
        }
    }

    if (!m_outputs.empty() && m_outputs.front()->layerOutput) {
        river_layer_shell_output_v1_set_default(m_outputs.front()->layerOutput);
    }

    for (auto& out : m_outputs) {
        std::vector<uint64_t> tiledIds;

        for (auto& win : m_windows) {
            if (win->outputId == 0) win->outputId = out->id;
            if (win->outputId != out->id) continue;

            const bool visible = (win->workspace == out->activeWorkspace);
            if (!visible) {
                river_window_v1_hide(win->wl);
                continue;
            }

            river_window_v1_show(win->wl);
            river_window_v1_use_ssd(win->wl);

            if (win->fullscreen) {
                river_window_v1_fullscreen(win->wl, out->wl);
                river_window_v1_set_tiled(win->wl, 0);
            } else {
                river_window_v1_exit_fullscreen(win->wl);
                if (win->floating) {
                    river_window_v1_set_tiled(win->wl, 0);
                    if (!win->floatInitialized) {
                        win->floatRect = Rect{
                            out->usableRect.x + out->usableRect.width / 6,
                            out->usableRect.y + out->usableRect.height / 6,
                            std::max(320, out->usableRect.width * 2 / 3),
                            std::max(240, out->usableRect.height * 2 / 3),
                        };
                        win->floatInitialized = true;
                    }
                    river_window_v1_propose_dimensions(win->wl, win->floatRect.width, win->floatRect.height);
                } else {
                    tiledIds.push_back(win->id);
                }
            }
        }

        const auto layout = LayoutEngine::computeDwindle(
            tiledIds,
            out->usableRect,
            m_config.gapsInner,
            m_config.gapsOuter,
            m_config.dwindleSplitRatio);

        for (const auto& item : layout) {
            if (auto* win = findWindow(item.id)) {
                win->tiledRect = item.rect;
                river_window_v1_set_tiled(win->wl, kAllEdges);
                river_window_v1_propose_dimensions(win->wl, item.rect.width, item.rect.height);
            }
        }
    }

    ensureValidFocusOnOutput(primaryOutput());
    WindowState* focused = focusedWindow();
    for (auto& seat : m_seats) {
        if (!seat->layerExclusiveFocus) {
            if (focused) {
                river_seat_v1_focus_window(seat->wl, focused->wl);
            } else {
                river_seat_v1_clear_focus(seat->wl);
            }
        }
    }

    river_window_manager_v1_manage_finish(m_wm);
    m_inManageSequence = false;
    broadcastIpcState();
}

void WindowManager::onRenderStart() {
    const Rgba32 activeCol = toPremultipliedWaylandRgba(m_palette.borderActive);
    const Rgba32 inactiveCol = toPremultipliedWaylandRgba(m_palette.borderInactive);
    const auto now = std::chrono::steady_clock::now();

    for (auto& out : m_outputs) {
        // Place tiled windows first, then floating windows above them
        for (int pass = 0; pass < 2; ++pass) {
            const bool wantFloating = (pass == 1);
            for (auto& win : m_windows) {
                if (win->outputId != out->id || win->workspace != out->activeWorkspace) continue;
                if (win->floating != wantFloating) continue;

                const Rect& r = win->fullscreen ? out->fullRect : (win->floating ? win->floatRect : win->tiledRect);

                // Start non-linear opening slide once the window has real dimensions
                if (!win->hasOpenedOnce && !win->fullscreen && win->currentW > 0 && win->currentH > 0) {
                    win->hasOpenedOnce = true;
                    win->animActive = true;
                    win->closingAnim = false;
                    win->animStart = now;
                    win->animDurationMs = 130;
                    win->animFromX = r.x;
                    win->animFromY = r.y + 28;
                }

                int drawX = r.x;
                int drawY = r.y;
                if (win->animActive && !win->fullscreen && win->currentW > 0 && win->currentH > 0) {
                    const double elapsedMs =
                        std::chrono::duration<double, std::milli>(now - win->animStart).count();
                    const double raw = std::clamp(elapsedMs / static_cast<double>(win->animDurationMs), 0.0, 1.0);

                    if (win->closingAnim) {
                        // Ease-in cubic: accelerates smoothly on exit
                        const double easedIn = std::pow(raw, 3.0);
                        drawX = r.x;
                        drawY = r.y + static_cast<int>(std::round(24.0 * easedIn));
                    } else {
                        // Ease-out exponential: snappy start, silky deceleration into place
                        const double easedOut = (raw >= 1.0) ? 1.0 : (1.0 - std::pow(2.0, -10.0 * raw));
                        drawX = static_cast<int>(std::round(win->animFromX + (r.x - win->animFromX) * easedOut));
                        drawY = static_cast<int>(std::round(win->animFromY + (r.y - win->animFromY) * easedOut));
                    }

                    if (raw >= 1.0) {
                        win->animActive = false;
                        if (win->closingAnim) {
                            win->pendingClose = true;
                            scheduleManageDirty();
                        }
                    }
                }

                river_node_v1_set_position(win->node, drawX, drawY);
                if (wantFloating) {
                    river_node_v1_place_top(win->node);
                }

                const bool isFocused = (win->id == m_focusedWindowId);
                const Rgba32& bc = isFocused ? activeCol : inactiveCol;
                river_window_v1_set_borders(
                    win->wl,
                    win->fullscreen ? 0 : kAllEdges,
                    win->fullscreen ? 0 : m_config.borderWidth,
                    bc.r, bc.g, bc.b, bc.a);

                // Rounded corner overlays above the window & borders
                const int S = m_corners.patchSize();
                const int bw = m_corners.borderWidth();
                const int w = win->currentW;
                const int h = win->currentH;
                const bool showCorners = m_corners.isReady() && !win->fullscreen &&
                                         !win->animActive && !win->closingAnim && !win->pendingClose &&
                                         w >= 2 * S && h >= 2 * S;
                if (showCorners) {
                    const int offsets[4][2] = {
                        {-bw, -bw},
                        {w + bw - S, -bw},
                        {-bw, h + bw - S},
                        {w + bw - S, h + bw - S},
                    };
                    const uint32_t scale120 = win->cornerScale120 ? win->cornerScale120 : 120u;
                    const int P = m_corners.pixelSize(scale120);
                    const bool needsCommit = !win->cornerMapped ||
                                             win->cornerActive != isFocused ||
                                             win->cornerVersion != m_corners.version() ||
                                             win->cornerLastScale120 != scale120;

                    for (int i = 0; i < 4; ++i) {
                        if (!win->cornerSurf[i]) {
                            win->cornerSurf[i] = m_corners.createClickThroughSurface();
                            if (win->cornerSurf[i]) {
                                win->cornerDec[i] = river_window_v1_get_decoration_above(
                                    win->wl, win->cornerSurf[i]);
                                if (m_viewporter) {
                                    win->cornerViewport[i] = wp_viewporter_get_viewport(
                                        m_viewporter, win->cornerSurf[i]);
                                }
                                if (i == 0 && m_fractionalScaleMgr) {
                                    win->cornerFracScale = wp_fractional_scale_manager_v1_get_fractional_scale(
                                        m_fractionalScaleMgr, win->cornerSurf[0]);
                                    wp_fractional_scale_v1_add_listener(
                                        win->cornerFracScale, &kCornerScaleListener, win.get());
                                }
                            }
                        }
                        if (!win->cornerDec[i] || !win->cornerSurf[i]) continue;

                        river_decoration_v1_set_offset(win->cornerDec[i], offsets[i][0], offsets[i][1]);
                        if (needsCommit) {
                            if (win->cornerViewport[i]) {
                                wp_viewport_set_destination(win->cornerViewport[i], S, S);
                            }
                            wl_surface_attach(win->cornerSurf[i], m_corners.buffer(isFocused, i, scale120), 0, 0);
                            wl_surface_damage_buffer(win->cornerSurf[i], 0, 0, P, P);
                            river_decoration_v1_sync_next_commit(win->cornerDec[i]);
                            wl_surface_commit(win->cornerSurf[i]);
                        }
                    }
                    win->cornerMapped = true;
                    win->cornerActive = isFocused;
                    win->cornerVersion = m_corners.version();
                    win->cornerLastScale120 = scale120;
                } else if (win->cornerMapped) {
                    for (int i = 0; i < 4; ++i) {
                        if (win->cornerDec[i] && win->cornerSurf[i]) {
                            if (win->cornerViewport[i]) {
                                wp_viewport_set_destination(win->cornerViewport[i], -1, -1);
                            }
                            wl_surface_attach(win->cornerSurf[i], nullptr, 0, 0);
                            river_decoration_v1_sync_next_commit(win->cornerDec[i]);
                            wl_surface_commit(win->cornerSurf[i]);
                        }
                    }
                    win->cornerMapped = false;
                }
            }
        }
    }

    if (auto* focused = focusedWindow()) {
        if (focused->floating) {
            river_node_v1_place_top(focused->node);
        }
    }

    river_window_manager_v1_render_finish(m_wm);
}

void WindowManager::beginPointerOp(SeatState* seat, PointerOpKind kind, uint64_t targetWinId) {
    if (!seat && !m_seats.empty()) seat = m_seats.front().get();
    if (!seat) return;

    uint64_t winId = targetWinId ? targetWinId : (seat->hoveredWindowId ? seat->hoveredWindowId : m_focusedWindowId);
    WindowState* win = findWindow(winId);
    if (!win || win->fullscreen) return;

    m_focusedWindowId = win->id;
    if (!win->floating) {
        win->floating = true;
        win->floatRect = win->tiledRect;
        win->floatInitialized = true;
    }

    seat->opWindowId = win->id;
    seat->opStartRect = win->floatRect;
    seat->opAccumDx = 0;
    seat->opAccumDy = 0;
    seat->pendingStartOp = kind;
    scheduleManageDirty();
}

void WindowManager::updatePointerOpDelta(SeatState* seat, int dx, int dy) {
    if (!seat || seat->activeOp == PointerOpKind::None) return;
    WindowState* win = findWindow(seat->opWindowId);
    if (!win) return;

    seat->opAccumDx += dx;
    seat->opAccumDy += dy;

    if (seat->activeOp == PointerOpKind::Move) {
        win->floatRect.x = seat->opStartRect.x + seat->opAccumDx;
        win->floatRect.y = seat->opStartRect.y + seat->opAccumDy;
    } else if (seat->activeOp == PointerOpKind::Resize) {
        win->floatRect.width = std::max(160, seat->opStartRect.width + seat->opAccumDx);
        win->floatRect.height = std::max(100, seat->opStartRect.height + seat->opAccumDy);
    }
    scheduleManageDirty();
}

void WindowManager::endPointerOp(SeatState* seat) {
    if (!seat) return;
    if (seat->activeOp != PointerOpKind::None || seat->pendingStartOp != PointerOpKind::None) {
        seat->pendingStartOp = PointerOpKind::None;
        seat->pendingEndOp = true;
        scheduleManageDirty();
    }
}

void WindowManager::onActionSwitchWorkspace(int ws) {
    if (m_sessionLocked || ws < 1 || ws > m_config.workspaceCount) return;
    if (auto* out = primaryOutput()) {
        out->activeWorkspace = ws;
        m_focusedWindowId = 0;
        ensureValidFocusOnOutput(out);
        scheduleManageDirty();
    }
}

void WindowManager::onActionMoveWindowToWorkspace(int ws) {
    if (ws < 1 || ws > m_config.workspaceCount) return;
    if (auto* win = focusedWindow()) {
        win->workspace = ws;
        if (auto* out = findOutput(win->outputId)) {
            out->activeWorkspace = ws;
        }
        scheduleManageDirty();
    }
}

void WindowManager::onActionCloseFocused() {
    if (auto* win = focusedWindow()) {
        if (!win->fullscreen && win->currentW > 0 && win->currentH > 0 && !win->closingAnim) {
            win->closingAnim = true;
            win->animActive = true;
            win->animStart = std::chrono::steady_clock::now();
            win->animDurationMs = 90;
            scheduleManageDirty();
        } else {
            river_window_v1_close(win->wl);
        }
    }
}

void WindowManager::onActionToggleFloatFocused() {
    if (auto* win = focusedWindow()) {
        win->floating = !win->floating;
        if (win->floating && !win->floatInitialized) {
            win->floatRect = win->tiledRect;
            win->floatInitialized = true;
        }
        scheduleManageDirty();
    }
}

void WindowManager::onActionToggleFullscreenFocused() {
    if (auto* win = focusedWindow()) {
        win->fullscreen = !win->fullscreen;
        scheduleManageDirty();
    }
}

void WindowManager::onActionFocusDirection(FocusDirection dir) {
    WindowState* cur = focusedWindow();
    OutputState* out = cur ? findOutput(cur->outputId) : primaryOutput();
    if (!out || out->removed) out = primaryOutput();
    if (!out) return;

    auto rectFor = [&](const WindowState* w) -> Rect {
        if (w->fullscreen) return out->fullRect;
        return w->floating ? w->floatRect : w->tiledRect;
    };

    if (!cur || cur->outputId != out->id || cur->workspace != out->activeWorkspace) {
        ensureValidFocusOnOutput(out);
        if (m_focusedWindowId != 0) scheduleManageDirty();
        return;
    }

    const Rect a = rectFor(cur);
    const int ax = a.x + a.width / 2;
    const int ay = a.y + a.height / 2;
    const int aLeft = a.x;
    const int aRight = a.x + a.width;
    const int aTop = a.y;
    const int aBottom = a.y + a.height;

    WindowState* best = nullptr;
    std::tuple<int, int, int, int> bestScore{};

    for (auto& w : m_windows) {
        if (w->id == cur->id) continue;
        if (w->outputId != out->id || w->workspace != out->activeWorkspace) continue;

        const Rect b = rectFor(w.get());
        const int bx = b.x + b.width / 2;
        const int by = b.y + b.height / 2;
        const int bLeft = b.x;
        const int bRight = b.x + b.width;
        const int bTop = b.y;
        const int bBottom = b.y + b.height;

        int primaryDelta = 0;
        int perpDelta = 0;
        int edgeDist = 0;
        bool hasOverlap = false;

        switch (dir) {
            case FocusDirection::Left:
                primaryDelta = ax - bx;
                perpDelta = std::abs(by - ay);
                edgeDist = std::max(0, aLeft - bRight);
                hasOverlap = std::min(aBottom, bBottom) > std::max(aTop, bTop);
                break;
            case FocusDirection::Right:
                primaryDelta = bx - ax;
                perpDelta = std::abs(by - ay);
                edgeDist = std::max(0, bLeft - aRight);
                hasOverlap = std::min(aBottom, bBottom) > std::max(aTop, bTop);
                break;
            case FocusDirection::Up:
                primaryDelta = ay - by;
                perpDelta = std::abs(bx - ax);
                edgeDist = std::max(0, aTop - bBottom);
                hasOverlap = std::min(aRight, bRight) > std::max(aLeft, bLeft);
                break;
            case FocusDirection::Down:
                primaryDelta = by - ay;
                perpDelta = std::abs(bx - ax);
                edgeDist = std::max(0, bTop - aBottom);
                hasOverlap = std::min(aRight, bRight) > std::max(aLeft, bLeft);
                break;
        }

        if (primaryDelta <= 0) continue;
        if (!hasOverlap && primaryDelta < perpDelta) continue;

        const auto score = std::make_tuple(hasOverlap ? 0 : 1, edgeDist, perpDelta, primaryDelta);
        if (!best || score < bestScore) {
            best = w.get();
            bestScore = score;
        }
    }

    if (best) {
        m_focusedWindowId = best->id;
        scheduleManageDirty();
    }
}

void WindowManager::onActionSpawnCommand(const std::string& cmd) {
    spawnDetached(cmd);
}

void WindowManager::onActionIpcCommand(const QJsonObject& cmd) {
    const QString action = cmd.value(QStringLiteral("cmd")).toString();
    if (action == QStringLiteral("switch_workspace")) {
        onActionSwitchWorkspace(cmd.value(QStringLiteral("workspace")).toInt(1));
    } else if (action == QStringLiteral("toggle_float")) {
        if (!m_sessionLocked) onActionToggleFloatFocused();
    } else if (action == QStringLiteral("close_focused")) {
        if (!m_sessionLocked) onActionCloseFocused();
    } else if (action == QStringLiteral("reload_config")) {
        reloadConfig();
    } else if (action == QStringLiteral("spawn_terminal")) {
        if (!m_sessionLocked) spawnDetached(resolveTerminalCommand(m_config.terminal));
    } else if (cmd.contains(QStringLiteral("event")) && m_ipc) {
        if (!m_sessionLocked || cmd.value(QStringLiteral("event")).toString() != QStringLiteral("toggle_launcher")) {
            m_ipc->broadcastState(cmd);
        }
    }
}

void WindowManager::reloadConfig() {
    m_config = ConfigStore::load();
    m_palette = paletteForName(m_config.themeName);
    m_corners.rebuild(m_config.cornerRadius, m_config.borderWidth, m_palette);
    if (m_outputScaleMgr) {
        m_outputScaleMgr->setDesiredScales(m_config.outputScales);
    }
    if (m_xkbConfig) {
        rebuildAndPushXkbKeymap();
    }
    for (auto& dev : m_libinputDevices) {
        applyLibinputConfig(dev.get());
    }
    scheduleManageDirty();
}

void WindowManager::rebuildAndPushXkbKeymap() {
    if (!m_xkbCtx || !m_xkbConfig) return;

    const std::string layouts = m_config.keyboardLayouts.toStdString();
    const std::string options = m_config.keyboardOptions.toStdString();
    xkb_rule_names names{
        .rules = nullptr,
        .model = nullptr,
        .layout = layouts.c_str(),
        .variant = nullptr,
        .options = options.c_str(),
    };
    xkb_keymap* km = xkb_keymap_new_from_names(m_xkbCtx, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (!km) return;

    char* str = xkb_keymap_get_as_string(km, XKB_KEYMAP_FORMAT_TEXT_V1);
    xkb_keymap_unref(km);
    if (!str) return;

    const int fd = createMemfdWithString(std::string(str));
    std::free(str);
    if (fd < 0) return;

    if (m_pendingKeymap) {
        river_xkb_keymap_v1_destroy(m_pendingKeymap);
        m_pendingKeymap = nullptr;
    }
    m_pendingKeymap = river_xkb_config_v1_create_keymap(
        m_xkbConfig, fd, RIVER_XKB_CONFIG_V1_KEYMAP_FORMAT_TEXT_V1);
    ::close(fd);
    river_xkb_keymap_v1_add_listener(
        m_pendingKeymap, &kXkbKeymapListener, new KeymapHook{this, m_pendingKeymap});
}

void WindowManager::onKeymapCreated(river_xkb_keymap_v1* km, bool ok, const char* errMsg) {
    if (km == m_pendingKeymap) {
        m_pendingKeymap = nullptr;
    }
    if (!ok) {
        std::cerr << "[lightwm-wm] XKB keymap error: " << (errMsg ? errMsg : "unknown") << "\n";
        river_xkb_keymap_v1_destroy(km);
        return;
    }
    if (m_activeKeymap) {
        river_xkb_keymap_v1_destroy(m_activeKeymap);
    }
    m_activeKeymap = km;
    m_keymapReady = true;
    applyXkbKeymapToKeyboards();
}

void WindowManager::applyXkbKeymapToKeyboards() {
    if (!m_activeKeymap || !m_keymapReady) return;
    for (auto& kbd : m_keyboards) {
        river_xkb_keyboard_v1_set_keymap(kbd->wl, m_activeKeymap);
    }
}

void WindowManager::scheduleManageDirty() {
    if (m_wm && !m_inManageSequence && !m_dirtyQueued) {
        river_window_manager_v1_manage_dirty(m_wm);
        m_dirtyQueued = true;
    }
}

bool WindowManager::hasActiveAnimations() const {
    for (const auto& w : m_windows) {
        if (w->animActive || w->pendingClose) return true;
    }
    return false;
}

int WindowManager::nextAnimationPollTimeoutMs() const {
    if (!hasActiveAnimations()) return -1;
    const auto now = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastAnimStep).count();
    if (elapsed >= 16) return 0;
    return static_cast<int>(16 - elapsed);
}

void WindowManager::stepAnimations() {
    if (!hasActiveAnimations()) return;
    const auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastAnimStep).count() >= 16) {
        m_lastAnimStep = now;
        scheduleManageDirty();
    }
}

void WindowManager::updateKeyboardLayoutDisplay(const std::string& layoutName) {
    m_keyboardLayout = layoutName;
    broadcastIpcState();
}

void WindowManager::broadcastIpcState() {
    if (!m_ipc) return;

    QJsonObject state;
    state.insert(QStringLiteral("type"), QStringLiteral("state"));
    state.insert(QStringLiteral("workspaceCount"), m_config.workspaceCount);
    state.insert(QStringLiteral("keyboardLayout"), QString::fromStdString(m_keyboardLayout));

    auto* out = primaryOutput();
    state.insert(QStringLiteral("activeWorkspace"), out ? out->activeWorkspace : 1);

    QJsonArray occupied;
    for (int ws = 1; ws <= m_config.workspaceCount; ++ws) {
        bool hasWin = false;
        for (const auto& w : m_windows) {
            if (w->workspace == ws) {
                hasWin = true;
                break;
            }
        }
        occupied.append(hasWin);
    }
    state.insert(QStringLiteral("occupiedWorkspaces"), occupied);

    WindowState* foc = focusedWindow();
    state.insert(QStringLiteral("activeTitle"), foc ? QString::fromStdString(foc->title) : QString());
    state.insert(QStringLiteral("activeAppId"), foc ? QString::fromStdString(foc->appId) : QString());
    state.insert(QStringLiteral("isFloating"), foc ? foc->floating : false);
    if (m_outputScaleMgr) {
        state.insert(QStringLiteral("outputs"), m_outputScaleMgr->outputsJson());
    }

    m_ipc->broadcastState(state);
}

OutputState* WindowManager::primaryOutput() {
    return m_outputs.empty() ? nullptr : m_outputs.front().get();
}

OutputState* WindowManager::findOutput(uint64_t id) {
    for (auto& out : m_outputs) {
        if (out->id == id) return out.get();
    }
    return nullptr;
}

WindowState* WindowManager::findWindow(uint64_t id) {
    for (auto& w : m_windows) {
        if (w->id == id) return w.get();
    }
    return nullptr;
}

WindowState* WindowManager::focusedWindow() {
    return findWindow(m_focusedWindowId);
}

void WindowManager::ensureValidFocusOnOutput(OutputState* out) {
    if (!out) return;
    if (auto* f = focusedWindow()) {
        if (f->outputId == out->id && f->workspace == out->activeWorkspace) {
            return;
        }
    }
    for (auto it = m_windows.rbegin(); it != m_windows.rend(); ++it) {
        if ((*it)->outputId == out->id && (*it)->workspace == out->activeWorkspace) {
            m_focusedWindowId = (*it)->id;
            return;
        }
    }
    m_focusedWindowId = 0;
}

} // namespace lightwm::wm
