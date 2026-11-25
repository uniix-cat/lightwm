import QtQuick
import QtQuick.Layouts
import org.kde.layershell 1.0 as LayerShell
import "../components"

Window {
    id: root
    property string osdLabel: ""
    property int osdValue: 0

    function showOsd(label, value) {
        osdLabel = label
        osdValue = value
        visible = true
        hideTimer.restart()
    }

    visible: false
    width: 240
    height: 54
    color: "transparent"

    LayerShell.Window.anchors: LayerShell.Window.AnchorBottom
    LayerShell.Window.layer: LayerShell.Window.LayerOverlay
    LayerShell.Window.margins: Qt.margins(0, 0, 0, 48)
    LayerShell.Window.keyboardInteractivity: LayerShell.Window.KeyboardInteractivityNone
    LayerShell.Window.scope: "lightwm-osd"

    Timer {
        id: hideTimer
        interval: 1400
        onTriggered: root.visible = false
    }

    ThemeSurface {
        anchors.fill: parent
        elevated: true
        border.color: Theme.borderActive

        RowLayout {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 10

            Text {
                text: root.osdLabel
                font.family: Theme.fontFamily
                font.pixelSize: 12
                font.bold: true
                color: Theme.textPrimary
            }

            Rectangle {
                Layout.fillWidth: true
                height: 8
                radius: 4
                color: Theme.borderInactive

                Rectangle {
                    width: parent.width * Math.min(1.0, Math.max(0.0, root.osdValue / 100.0))
                    height: parent.height
                    radius: 4
                    color: Theme.accentStrong
                }
            }

            Text {
                text: root.osdValue + "%"
                font.family: Theme.fontFamily
                font.pixelSize: 12
                color: Theme.textSecondary
            }
        }
    }
}
