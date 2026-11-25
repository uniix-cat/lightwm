import QtQuick
import QtQuick.Layouts
import org.kde.layershell 1.0 as LayerShell
import "../components"

Window {
    id: barWin
    required property var targetScreen
    signal toggleLauncher()
    signal toggleControlCenter()
    signal requestScreenshot()
    signal requestLock()

    screen: targetScreen
    visible: false
    color: "transparent"
    width: targetScreen ? targetScreen.width : 1280
    height: Theme.barHeight + 12

    LayerShell.Window.screen: targetScreen
    LayerShell.Window.anchors: LayerShell.Window.AnchorTop | LayerShell.Window.AnchorLeft | LayerShell.Window.AnchorRight
    LayerShell.Window.layer: LayerShell.Window.LayerTop
    LayerShell.Window.exclusionZone: Theme.barHeight + 8
    LayerShell.Window.keyboardInteractivity: LayerShell.Window.KeyboardInteractivityNone
    LayerShell.Window.scope: "lightwm-bar"

    Component.onCompleted: visible = true

    ThemeSurface {
        anchors.fill: parent
        anchors.topMargin: 6
        anchors.bottomMargin: 2
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        radius: Theme.radius

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            spacing: 10

            PillButton {
                iconGlyph: String.fromCodePoint(0xF003B)
                text: "Apps"
                onClicked: barWin.toggleLauncher()
            }

            WorkspacesWidget {}

            Text {
                Layout.fillWidth: true
                text: Wm.activeTitle.length > 0 ? Wm.activeTitle : "LightWM"
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignHCenter
                font.family: Theme.fontFamily
                font.pixelSize: 12
                color: Theme.textSecondary
            }

            TrayWidget {}

            StatusCluster {
                onControlCenterClicked: barWin.toggleControlCenter()
                onScreenshotClicked: barWin.requestScreenshot()
                onLockClicked: barWin.requestLock()
            }
        }
    }
}
