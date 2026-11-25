#pragma once

#include <functional>

struct libinput;
struct udev;

namespace lightwm::wm {

// Reads 3-finger touchpad swipe gestures via libinput to switch workspaces.
class TouchpadGestures {
public:
    using SwipeCallback = std::function<void(int direction)>; // -1 = prev workspace, +1 = next workspace

    explicit TouchpadGestures(SwipeCallback onSwipe);
    ~TouchpadGestures();

    bool init();
    void shutdown();
    int fd() const;
    bool hasGestureDevice() const { return m_hasGestureDevice; }
    void processEvents();

private:
    SwipeCallback m_onSwipe;
    udev* m_udev = nullptr;
    libinput* m_li = nullptr;
    bool m_hasGestureDevice = false;
    int m_activeFingers = 0;
    double m_dx = 0.0;
    double m_dy = 0.0;
    bool m_triggered = false;
};

} // namespace lightwm::wm
