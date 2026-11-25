#include "wm/TouchpadGestures.hpp"

#include <cerrno>
#include <cmath>
#include <fcntl.h>
#include <libinput.h>
#include <libudev.h>
#include <unistd.h>

namespace lightwm::wm {

namespace {

int openRestricted(const char* path, int flags, void*) {
    int fd = ::open(path, flags | O_CLOEXEC);
    return fd < 0 ? -errno : fd;
}

void closeRestricted(int fd, void*) {
    if (fd >= 0) ::close(fd);
}

const libinput_interface kLibinputIf = {
    .open_restricted = openRestricted,
    .close_restricted = closeRestricted,
};

} // namespace

TouchpadGestures::TouchpadGestures(SwipeCallback onSwipe)
    : m_onSwipe(std::move(onSwipe)) {}

TouchpadGestures::~TouchpadGestures() {
    shutdown();
}

bool TouchpadGestures::init() {
    shutdown();
    m_udev = udev_new();
    if (!m_udev) return false;

    m_li = libinput_udev_create_context(&kLibinputIf, nullptr, m_udev);
    if (!m_li) {
        shutdown();
        return false;
    }

    if (libinput_udev_assign_seat(m_li, "seat0") != 0) {
        shutdown();
        return false;
    }

    libinput_dispatch(m_li);
    processEvents();
    return true;
}

void TouchpadGestures::shutdown() {
    m_hasGestureDevice = false;
    if (m_li) {
        libinput_unref(m_li);
        m_li = nullptr;
    }
    if (m_udev) {
        udev_unref(m_udev);
        m_udev = nullptr;
    }
}

int TouchpadGestures::fd() const {
    return m_li ? libinput_get_fd(m_li) : -1;
}

void TouchpadGestures::processEvents() {
    if (!m_li) return;
    libinput_dispatch(m_li);

    libinput_event* ev = nullptr;
    while ((ev = libinput_get_event(m_li)) != nullptr) {
        const auto type = libinput_event_get_type(ev);

        if (type == LIBINPUT_EVENT_DEVICE_ADDED) {
            auto* dev = libinput_event_get_device(ev);
            if (libinput_device_has_capability(dev, LIBINPUT_DEVICE_CAP_GESTURE)) {
                m_hasGestureDevice = true;
            }
        } else if (type == LIBINPUT_EVENT_GESTURE_SWIPE_BEGIN) {
            auto* g = libinput_event_get_gesture_event(ev);
            m_activeFingers = libinput_event_gesture_get_finger_count(g);
            m_dx = 0.0;
            m_dy = 0.0;
            m_triggered = false;
        } else if (type == LIBINPUT_EVENT_GESTURE_SWIPE_UPDATE) {
            auto* g = libinput_event_get_gesture_event(ev);
            if (m_activeFingers == 3 && !m_triggered) {
                m_dx += libinput_event_gesture_get_dx(g);
                m_dy += libinput_event_gesture_get_dy(g);

                constexpr double kSwipeThreshold = 75.0;
                if (std::abs(m_dx) >= kSwipeThreshold && std::abs(m_dx) > std::abs(m_dy) * 1.35) {
                    m_triggered = true;
                    if (m_onSwipe) {
                        // Swipe left -> next workspace (+1), swipe right -> previous workspace (-1)
                        m_onSwipe(m_dx < 0 ? +1 : -1);
                    }
                }
            }
        } else if (type == LIBINPUT_EVENT_GESTURE_SWIPE_END) {
            m_activeFingers = 0;
            m_dx = 0.0;
            m_dy = 0.0;
            m_triggered = false;
        }

        libinput_event_destroy(ev);
    }
}

} // namespace lightwm::wm
