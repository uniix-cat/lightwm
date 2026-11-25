import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    property string iconGlyph: ""
    property string text: ""
    property bool active: false
    property bool danger: false
    signal clicked()

    implicitWidth: contentRow.implicitWidth + 18
    implicitHeight: 28
    radius: Math.min(Theme.radius, height / 2)
    color: {
        if (danger) return mouse.pressed ? Qt.darker(Theme.danger, 1.15) : Theme.danger
        if (active) return mouse.containsMouse ? Theme.accentStrong : Theme.accent
        return mouse.containsMouse ? Theme.surfaceElevated : Theme.surface
    }
    border.width: 1
    border.color: active ? Theme.accentStrong : Theme.borderInactive

    RowLayout {
        id: contentRow
        anchors.centerIn: parent
        spacing: 5

        Text {
            visible: root.iconGlyph.length > 0
            text: root.iconGlyph
            font.family: Theme.iconFontFamily
            font.pixelSize: 13
            color: (root.active || root.danger) ? Theme.textOnAccent : Theme.textPrimary
        }

        Text {
            visible: root.text.length > 0
            text: root.text
            font.family: Theme.fontFamily
            font.pixelSize: 12
            font.weight: root.active ? Font.DemiBold : Font.Normal
            color: (root.active || root.danger) ? Theme.textOnAccent : Theme.textPrimary
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
