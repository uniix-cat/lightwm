import QtQuick
import "wallpaper"
import "bar"
import "popovers"
import "launcher"
import "notifications"
import "screenshot"

QtObject {
    id: root

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
            onToggleLauncher: launcherOverlay.visible = !launcherOverlay.visible
            onToggleControlCenter: controlCenter.visible = !controlCenter.visible
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
            launcherOverlay.visible = !launcherOverlay.visible
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
