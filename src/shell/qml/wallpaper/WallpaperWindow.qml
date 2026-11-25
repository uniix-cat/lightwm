import QtQuick
import org.kde.layershell 1.0 as LayerShell

Window {
    id: wpWin
    required property var targetScreen

    screen: targetScreen
    visible: false
    color: Theme.background
    width: targetScreen ? targetScreen.width : 1280
    height: targetScreen ? targetScreen.height : 720

    LayerShell.Window.screen: targetScreen
    LayerShell.Window.anchors: LayerShell.Window.AnchorTop | LayerShell.Window.AnchorBottom | LayerShell.Window.AnchorLeft | LayerShell.Window.AnchorRight
    LayerShell.Window.layer: LayerShell.Window.LayerBackground
    LayerShell.Window.exclusionZone: -1
    LayerShell.Window.keyboardInteractivity: LayerShell.Window.KeyboardInteractivityNone
    LayerShell.Window.scope: "lightwm-wallpaper"

    Component.onCompleted: visible = true

    // Built-in minimal theme gradient when no image is configured
    Rectangle {
        anchors.fill: parent
        visible: Theme.wallpaperPath.length === 0
        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0.0; color: Theme.background }
            GradientStop { position: 0.72; color: Theme.surface }
            GradientStop { position: 1.0; color: Qt.tint(Theme.surface, Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.25)) }
        }
    }

    Image {
        anchors.fill: parent
        visible: Theme.wallpaperPath.length > 0
        source: Theme.wallpaperPath.length > 0 ? "file://" + Theme.wallpaperPath : ""
        asynchronous: true
        smooth: true
        fillMode: Theme.wallpaperMode === "fit" ? Image.PreserveAspectFit
                : Theme.wallpaperMode === "stretch" ? Image.Stretch
                : Theme.wallpaperMode === "center" ? Image.Pad
                : Image.PreserveAspectCrop
    }
}
