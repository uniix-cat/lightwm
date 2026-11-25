import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Window {
    id: root
    property string errorText: ""

    visible: false
    color: Theme.background

    onVisibleChanged: {
        if (visible) {
            errorText = ""
            passField.text = ""
            passField.forceActiveFocus()
        }
    }

    Connections {
        target: Lockscreen
        function onUnlockFailed(reason) {
            root.errorText = reason
            passField.text = ""
            passField.forceActiveFocus()
        }
    }

    // Match user's wallpaper or theme gradient behind the lock card
    Rectangle {
        anchors.fill: parent
        visible: Theme.wallpaperPath.length === 0
        gradient: Gradient {
            GradientStop { position: 0.0; color: Theme.background }
            GradientStop { position: 1.0; color: Theme.surface }
        }
    }

    Image {
        anchors.fill: parent
        visible: Theme.wallpaperPath.length > 0
        source: Theme.wallpaperPath.length > 0 ? "file://" + Theme.wallpaperPath : ""
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
    }

    Rectangle {
        anchors.fill: parent
        color: "#66000000"
    }

    ThemeSurface {
        anchors.centerIn: parent
        width: 340
        height: 230
        elevated: true
        border.width: 2
        border.color: Theme.borderActive

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 24
            spacing: 12

            Text {
                id: clockLabel
                text: Qt.formatDateTime(new Date(), "HH:mm")
                font.family: Theme.fontFamily
                font.pixelSize: 38
                font.bold: true
                color: Theme.textPrimary
                Layout.alignment: Qt.AlignHCenter
            }

            Timer {
                interval: 10000
                running: root.visible
                repeat: true
                onTriggered: clockLabel.text = Qt.formatDateTime(new Date(), "HH:mm")
            }

            Text {
                text: Lockscreen.username
                font.family: Theme.fontFamily
                font.pixelSize: 13
                color: Theme.textSecondary
                Layout.alignment: Qt.AlignHCenter
            }

            TextField {
                id: passField
                Layout.fillWidth: true
                placeholderText: "Password"
                echoMode: TextInput.Password
                color: Theme.textPrimary
                onAccepted: Lockscreen.verifyPassword(text)

                background: Rectangle {
                    radius: Theme.radius - 2
                    color: Theme.surface
                    border.width: 1
                    border.color: Theme.borderActive
                }
            }

            Text {
                visible: root.errorText.length > 0
                text: root.errorText
                color: Theme.danger
                font.family: Theme.fontFamily
                font.pixelSize: 11
                Layout.alignment: Qt.AlignHCenter
            }
        }
    }
}
