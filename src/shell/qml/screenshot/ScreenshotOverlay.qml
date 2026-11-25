import QtQuick
import QtQuick.Layouts
import org.kde.layershell 1.0 as LayerShell
import "../components"

Window {
    id: root
    property point startPt: Qt.point(0, 0)
    property point currPt: Qt.point(0, 0)
    property bool dragging: false

    visible: false
    color: "#44000000"

    LayerShell.Window.anchors: LayerShell.Window.AnchorTop | LayerShell.Window.AnchorBottom | LayerShell.Window.AnchorLeft | LayerShell.Window.AnchorRight
    LayerShell.Window.layer: LayerShell.Window.LayerOverlay
    LayerShell.Window.exclusionZone: -1
    LayerShell.Window.keyboardInteractivity: LayerShell.Window.KeyboardInteractivityExclusive
    LayerShell.Window.scope: "lightwm-screenshot"

    readonly property int selX: Math.min(startPt.x, currPt.x)
    readonly property int selY: Math.min(startPt.y, currPt.y)
    readonly property int selW: Math.abs(currPt.x - startPt.x)
    readonly property int selH: Math.abs(currPt.y - startPt.y)

    Rectangle {
        visible: root.dragging && root.selW > 2 && root.selH > 2
        x: root.selX
        y: root.selY
        width: root.selW
        height: root.selH
        color: "#22FFFFFF"
        border.width: 2
        border.color: Theme.accentStrong
        radius: 4
    }

    ThemeSurface {
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 20
        width: toolbar.implicitWidth + 24
        height: 44
        elevated: true

        RowLayout {
            id: toolbar
            anchors.centerIn: parent
            spacing: 8
            Text {
                text: "Drag region to capture"
                color: Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: 12
            }
            PillButton {
                text: "Full Screen"
                active: true
                onClicked: {
                    root.visible = false
                    Screenshot.captureFullScreen()
                }
            }
            PillButton {
                text: "Cancel"
                onClicked: root.visible = false
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        z: -1
        cursorShape: Qt.CrossCursor
        onPressed: (mouse) => {
            root.startPt = Qt.point(mouse.x, mouse.y)
            root.currPt = Qt.point(mouse.x, mouse.y)
            root.dragging = true
        }
        onPositionChanged: (mouse) => {
            if (root.dragging) {
                root.currPt = Qt.point(mouse.x, mouse.y)
            }
        }
        onReleased: {
            if (root.dragging) {
                root.dragging = false
                const rx = Math.round(root.selX)
                const ry = Math.round(root.selY)
                const rw = Math.round(root.selW)
                const rh = Math.round(root.selH)
                root.visible = false
                if (rw > 4 && rh > 4) {
                    Screenshot.captureRegion(rx, ry, rw, rh)
                }
            }
        }
    }
}
