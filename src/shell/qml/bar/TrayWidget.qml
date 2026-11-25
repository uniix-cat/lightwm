import QtQuick
import QtQuick.Layouts

RowLayout {
    spacing: 6
    visible: Tray.items.length > 0

    Repeater {
        model: Tray.items
        delegate: Rectangle {
            required property var modelData
            width: 24
            height: 24
            radius: 6
            color: mouse.containsMouse ? Theme.surfaceElevated : "transparent"

            Text {
                anchors.centerIn: parent
                text: modelData.title.slice(0, 2).toUpperCase()
                font.family: Theme.fontFamily
                font.pixelSize: 10
                color: Theme.textPrimary
            }

            MouseArea {
                id: mouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: Tray.activateItem(modelData.service, modelData.path)
            }
        }
    }
}
