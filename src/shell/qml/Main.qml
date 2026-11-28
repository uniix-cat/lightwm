import QtQuick
import "wallpaper"
import "bar"
import "popovers"
import "launcher"
import "notifications"
import "screenshot"

QtObject {
    id: root
    property var pendingWindow: null

    function toggleWithBackdrop(win) {
        if (win.visible) {
            win.visible = false
            return
        }
        if (Theme.blurEnabled && Theme.blurStrength > 0 && Backdrop.available) {
            pendingWindow = win
            Backdrop.capturePrimary()
            backdropFallbackTimer.restart()
        } else {
            win.visible = true
        }
    }

    property Timer backdropFallbackTimer: Timer {
        interval: 40
        repeat: false
        onTriggered: {
            if (root.pendingWindow) {
                root.pendingWindow.visible = true
                root.pendingWindow = null
            }
        }
    }

    property Connections backdropConnections: Connections {
        target: Backdrop
        function onBackdropUpdated() {
            if (root.pendingWindow) {
                backdropFallbackTimer.stop()
                root.pendingWindow.visible = true
                root.pendingWindow = null
            }
        }
    }

    property Instantiator wallpapers: Instantiator {
        model: Qt.application.screens
        delegate: WallpaperWindow {
            required property var modelData
            targetScreen: modelData
        }
    }

    property Instantiator bars: Instantiator {
        model: Qt.application.screens
        delegate: BarWindow {
            required property var modelData
            targetScreen: modelData
            onToggleLauncher: root.toggleWithBackdrop(launcherOverlay)
            onToggleControlCenter: root.toggleWithBackdrop(controlCenter)
            onRequestScreenshot: screenshotOverlay.visible = true
            onRequestLock: Lockscreen.lock()
        }
    }

    property ControlCenterPopover controlCenter: ControlCenterPopover {}
    property LauncherOverlay launcherOverlay: LauncherOverlay {}
    property NotificationStack notificationStack: NotificationStack {}
    property OsdOverlay osdOverlay: OsdOverlay {}
    property ScreenshotOverlay screenshotOverlay: ScreenshotOverlay {}

    property Connections wmConnections: Connections {
        target: Wm
        function onToggleLauncherRequested() {
            root.toggleWithBackdrop(launcherOverlay)
        }
        function onScreenshotRequested() {
            screenshotOverlay.visible = true
        }
    }

    property Connections audioConnections: Connections {
        target: Audio
        function onOsdRequested(volume, muted) {
            osdOverlay.showOsd(muted ? "Muted" : "Volume", volume)
        }
    }

    property Connections brightnessConnections: Connections {
        target: Brightness
        function onOsdRequested(val) {
            osdOverlay.showOsd("Bright", val)
        }
    }
}
