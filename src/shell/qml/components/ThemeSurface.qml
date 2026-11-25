import QtQuick

Rectangle {
    property bool elevated: false
    color: elevated ? Theme.surfaceElevated : Theme.surface
    radius: Theme.radius
    border.width: 1
    border.color: Theme.borderInactive
}
