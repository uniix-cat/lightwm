import QtQuick

Row {
    spacing: 5

    Repeater {
        model: Wm.workspaceCount
        delegate: Rectangle {
            required property int index
            readonly property int ws: index + 1
            readonly property bool isActive: Wm.activeWorkspace === ws
            readonly property bool isOccupied: index < Wm.occupiedWorkspaces.length ? Wm.occupiedWorkspaces[index] : false

            width: isActive ? 30 : 22
            height: 22
            radius: 11
            color: isActive ? Theme.accent : (isOccupied ? Theme.surfaceElevated : "transparent")
            border.width: 1
            border.color: isActive ? Theme.accentStrong : (isOccupied ? Theme.borderInactive : "transparent")

            Behavior on width { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
            Behavior on color { ColorAnimation { duration: 140 } }

            Text {
                anchors.centerIn: parent
                text: ws
                font.family: Theme.fontFamily
                font.pixelSize: 11
                font.weight: isActive ? Font.Bold : Font.Medium
                color: isActive ? Theme.textOnAccent : Theme.textPrimary
            }

            Rectangle {
                visible: !isActive && isOccupied
                width: 4
                height: 4
                radius: 2
                color: Theme.accentStrong
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 2
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: Wm.switchWorkspace(ws)
            }
        }
    }
}
