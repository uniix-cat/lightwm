import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    id: root
    property string label: ""
    property int value: 50
    property int from: 0
    property int to: 100
    signal movedValue(int newValue)

    spacing: 10

    Text {
        text: root.label
        font.family: Theme.fontFamily
        font.pixelSize: 12
        color: Theme.textPrimary
        Layout.preferredWidth: 74
    }

    Slider {
        id: slider
        Layout.fillWidth: true
        from: root.from
        to: root.to
        value: root.value
        stepSize: 1
        onMoved: root.movedValue(Math.round(value))

        background: Rectangle {
            x: slider.leftPadding
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            width: slider.availableWidth
            height: 6
            radius: 3
            color: Theme.borderInactive

            Rectangle {
                width: slider.visualPosition * parent.width
                height: parent.height
                radius: 3
                color: Theme.accentStrong
            }
        }

        handle: Rectangle {
            x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            width: 16
            height: 16
            radius: 8
            color: Theme.accent
            border.width: 2
            border.color: Theme.accentStrong
        }
    }

    Text {
        text: root.value + "%"
        font.family: Theme.fontFamily
        font.pixelSize: 12
        color: Theme.textSecondary
        Layout.preferredWidth: 38
        horizontalAlignment: Text.AlignRight
    }
}
