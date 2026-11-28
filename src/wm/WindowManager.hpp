#pragma once

#include "core/Config.hpp"
#include "core/Theme.hpp"
#include "wm/CornerDecorations.hpp"
#include "wm/IpcServer.hpp"
#include "wm/LayoutEngine.hpp"
#include "wm/OutputScaleManager.hpp"
#include "wm/TouchpadGestures.hpp"

#include <QJsonObject>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct wl_compositor;
struct wl_display;
struct wl_registry;
struct wl_shm;
struct wl_surface;
struct wp_fractional_scale_manager_v1;
struct wp_fractional_scale_v1;
struct wp_viewporter;
struct wp_viewport;
struct xkb_context;

struct river_decoration_v1;
struct river_window_manager_v1;
struct river_window_v1;
struct river_node_v1;
struct river_output_v1;
struct river_seat_v1;
struct river_pointer_binding_v1;
struct river_layer_shell_v1;
struct river_layer_shell_output_v1;
struct river_layer_shell_seat_v1;
struct river_xkb_bindings_v1;
struct river_xkb_binding_v1;
struct river_xkb_config_v1;
struct river_xkb_keymap_v1;
struct river_xkb_keyboard_v1;
struct river_input_manager_v1;
struct river_libinput_config_v1;
struct river_libinput_device_v1;

namespace lightwm::wm {

enum class PointerOpKind {
    None,
    Move,
    Resize,
};

struct OutputState {
    uint64_t id = 0;
    river_output_v1* wl = nullptr;
    river_layer_shell_output_v1* layerOutput = nullptr;
    uint32_t wlOutputName = 0;

    Rect fullRect{0, 0, 1920, 1080};
    Rect usableRect{0, 0, 1920, 1080};
    bool hasNonExclusive = false;

    int activeWorkspace = 1;
    bool removed = false;
};

struct WindowState {
    struct WindowManager* wm = nullptr;
    uint64_t id = 0;
    river_window_v1* wl = nullptr;
    river_node_v1* node = nullptr;

    std::string appId;
    std::string title;
    uint64_t outputId = 0;
    int workspace = 1;

    bool floating = false;
    bool fullscreen = false;
    bool closed = false;

    int currentW = 0;
    int currentH = 0;
    Rect tiledRect{0, 0, 800, 600};
    Rect floatRect{160, 120, 960, 640};
    bool floatInitialized = false;

    // Animation state (content clip + slide via river_window_v1::set_content_clip_box)
    bool hasOpenedOnce = false;
    bool animActive = false;
    bool closingAnim = false;
    bool pendingClose = false;
    std::chrono::steady_clock::time_point animStart{};
    int animDurationMs = 140;
    int animFromX = 0;
    int animFromY = 0;

    // Rounded corner overlays via river_decoration_v1
    wl_surface* cornerSurf[4]{};
    river_decoration_v1* cornerDec[4]{};
    wp_viewport* cornerViewport[4]{};
    wp_fractional_scale_v1* cornerFracScale = nullptr;
    uint32_t cornerScale120 = 120;
    uint32_t cornerLastScale120 = 0;
    bool cornerMapped = false;
    bool cornerActive = false;
    uint64_t cornerVersion = 0;
};

struct SeatState {
    struct WindowManager* wm = nullptr;
    river_seat_v1* wl = nullptr;
    river_layer_shell_seat_v1* layerSeat = nullptr;
    uint32_t wlSeatName = 0;

    uint64_t hoveredWindowId = 0;
    bool layerExclusiveFocus = false;

    PointerOpKind activeOp = PointerOpKind::None;
    PointerOpKind pendingStartOp = PointerOpKind::None;
    bool pendingEndOp = false;
    uint64_t opWindowId = 0;
    Rect opStartRect{};
    int opAccumDx = 0;
    int opAccumDy = 0;

    std::vector<river_xkb_binding_v1*> xkbBindings;
    std::vector<river_pointer_binding_v1*> pointerBindings;
    bool bindingsCreated = false;
    bool bindingsDirty = true;
};

struct XkbKeyboardState {
    struct WindowManager* wm = nullptr;
    river_xkb_keyboard_v1* wl = nullptr;
    uint32_t currentLayoutIndex = 0;
    std::string currentLayoutName = "us";
};

struct LibinputDeviceState {
    struct WindowManager* wm = nullptr;
    river_libinput_device_v1* wl = nullptr;
    int tapFingerCount = 0;
    int naturalScrollSupported = 0;
    uint32_t tapCurrent = UINT32_MAX;
    uint32_t naturalScrollCurrent = UINT32_MAX;
    double accelSpeedCurrent = 0.0;
    bool hasAccelSpeedCurrent = false;
    bool accelSpeedSupported = false;
    bool configured = false;
    bool removed = false;
};

class WindowManager {
public:
    WindowManager();
    ~WindowManager();

    bool init(const std::string& ipcSocketPath);
    int run();
    void requestQuit() { m_running = false; }

    void reloadConfig();

    // River protocol callbacks
    void onRegistryGlobal(uint32_t name, const char* interface, uint32_t version);
    void onManageStart();
    void onRenderStart();
    void onNewWindow(river_window_v1* win);
    void onNewOutput(river_output_v1* out);
    void onNewSeat(river_seat_v1* seat);
    void onNewXkbKeyboard(river_xkb_keyboard_v1* kbd);
    void onNewLibinputDevice(river_libinput_device_v1* dev);

    void onWindowClosed(WindowState* win);
    void onWindowFullscreenRequested(WindowState* win, river_output_v1* output);
    void onOutputRemoved(OutputState* out);
    void onSeatRemoved(SeatState* seat);

    void onActionSwitchWorkspace(int ws);
    void onActionMoveWindowToWorkspace(int ws);
    void onActionCloseFocused();
    void onActionToggleFloatFocused();
    void onActionToggleFullscreenFocused();
    enum class FocusDirection { Left, Down, Up, Right };
    void onActionFocusDirection(FocusDirection dir);
    void onActionSpawnCommand(const std::string& cmd);
    void onActionIpcCommand(const QJsonObject& cmd);

    void beginPointerOp(SeatState* seat, PointerOpKind kind, uint64_t targetWinId = 0);
    void updatePointerOpDelta(SeatState* seat, int dx, int dy);
    void endPointerOp(SeatState* seat);

    void setSessionLocked(bool locked) { m_sessionLocked = locked; }
    void onKeymapCreated(river_xkb_keymap_v1* km, bool ok, const char* errMsg);

    void scheduleManageDirty();
    bool hasActiveAnimations() const;
    int nextAnimationPollTimeoutMs() const;
    void stepAnimations();
    void updateKeyboardLayoutDisplay(const std::string& layoutName);
    void applyLibinputConfig(LibinputDeviceState* dev);
    void onLibinputDeviceRemoved(LibinputDeviceState* dev);
    void onXkbKeyboardRemoved(XkbKeyboardState* kbd);

    enum class BindingAction {
        SpawnTerminal,
        CloseFocused,
        ToggleFloat,
        ToggleFullscreen,
        FocusLeft,
        FocusDown,
        FocusUp,
        FocusRight,
        ToggleLauncher,
        LockScreen,
        Screenshot,
        AudioRaise,
        AudioLower,
        AudioMute,
        BrightnessUp,
        BrightnessDown,
        SwitchWorkspace1,
        SwitchWorkspace2,
        SwitchWorkspace3,
        SwitchWorkspace4,
        SwitchWorkspace5,
        SwitchWorkspace6,
        SwitchWorkspace7,
        SwitchWorkspace8,
        SwitchWorkspace9,
        MoveWorkspace1,
        MoveWorkspace2,
        MoveWorkspace3,
        MoveWorkspace4,
        MoveWorkspace5,
        MoveWorkspace6,
        MoveWorkspace7,
        MoveWorkspace8,
        MoveWorkspace9,
    };

    struct XkbBindingData {
        WindowManager* wm = nullptr;
        SeatState* seat = nullptr;
        BindingAction action{};
        river_xkb_binding_v1* wl = nullptr;
    };

    struct PointerBindingData {
        WindowManager* wm = nullptr;
        SeatState* seat = nullptr;
        PointerOpKind op = PointerOpKind::None;
        river_pointer_binding_v1* wl = nullptr;
    };

private:
    void setupSeatBindings(SeatState* seat);
    void rebuildAndPushXkbKeymap();
    void applyXkbKeymapToKeyboards();
    void broadcastIpcState();

    OutputState* primaryOutput();
    OutputState* findOutput(uint64_t id);
    WindowState* findWindow(uint64_t id);
    WindowState* focusedWindow();
    void ensureValidFocusOnOutput(OutputState* out);

    wl_display* m_display = nullptr;
    wl_registry* m_registry = nullptr;
    wl_compositor* m_compositor = nullptr;
    wl_shm* m_shm = nullptr;
    wp_viewporter* m_viewporter = nullptr;
    wp_fractional_scale_manager_v1* m_fractionalScaleMgr = nullptr;
    xkb_context* m_xkbCtx = nullptr;

    river_window_manager_v1* m_wm = nullptr;
    river_layer_shell_v1* m_layerShell = nullptr;
    river_xkb_bindings_v1* m_xkbBindings = nullptr;
    river_xkb_config_v1* m_xkbConfig = nullptr;
    river_input_manager_v1* m_inputMgr = nullptr;
    river_libinput_config_v1* m_libinputCfg = nullptr;
    river_xkb_keymap_v1* m_activeKeymap = nullptr;
    river_xkb_keymap_v1* m_pendingKeymap = nullptr;
    bool m_keymapReady = false;

    ConfigData m_config;
    ThemePalette m_palette;

    std::vector<std::unique_ptr<OutputState>> m_outputs;
    std::vector<std::unique_ptr<WindowState>> m_windows;
    std::vector<std::unique_ptr<SeatState>> m_seats;
    std::vector<std::unique_ptr<XkbKeyboardState>> m_keyboards;
    std::vector<std::unique_ptr<LibinputDeviceState>> m_libinputDevices;
    std::vector<std::unique_ptr<XkbBindingData>> m_xkbBindingData;
    std::vector<std::unique_ptr<PointerBindingData>> m_pointerBindingData;

    uint64_t m_nextId = 1;
    uint64_t m_focusedWindowId = 0;
    std::string m_keyboardLayout = "us";
    bool m_running = true;
    bool m_sessionLocked = false;
    bool m_inManageSequence = false;
    bool m_dirtyQueued = false;

    std::unique_ptr<IpcServer> m_ipc;
    std::unique_ptr<TouchpadGestures> m_gestures;
    std::unique_ptr<OutputScaleManager> m_outputScaleMgr;
    CornerDecorations m_corners;
    int m_gestureRetryCount = 0;
    std::chrono::steady_clock::time_point m_lastAnimStep{};
};

} // namespace lightwm::wm
