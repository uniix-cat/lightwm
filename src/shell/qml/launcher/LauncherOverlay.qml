import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.layershell 1.0 as LayerShell
import "../components"

Window {
    id: root
    property string query: ""

    visible: false
    width: 520
    height: 420
    color: "transparent"

    LayerShell.Window.anchors: LayerShell.Window.AnchorNone
    LayerShell.Window.layer: LayerShell.Window.LayerOverlay
    LayerShell.Window.keyboardInteractivity: LayerShell.Window.KeyboardInteractivityExclusive
    LayerShell.Window.scope: "lightwm-launcher"

    onVisibleChanged: {
        if (visible) {
            query = ""
            searchField.text = ""
            searchField.forceActiveFocus()
        }
    }

    ThemeSurface {
        anchors.fill: parent
        elevated: true
        radius: Theme.radius
        border.width: 2
        border.color: Theme.borderActive

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 12

            TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: "Search applications..."
                font.family: Theme.fontFamily
                font.pixelSize: 14
                color: Theme.textPrimary
                onTextChanged: root.query = text
                Keys.onEscapePressed: root.visible = false
                onAccepted: {
                    const list = Launcher.search(root.query)
                    if (list.length > 0) {
                        Launcher.launch(list[0].exec)
                        root.visible = false
                    }
                }

                background: Rectangle {
                    radius: Theme.radius - 2
                    color: Theme.surface
                    border.width: 1
                    border.color: Theme.borderActive
                }
            }

            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 6
                model: Launcher.search(root.query)
                delegate: Rectangle {
                    required property var modelData
                    width: ListView.view.width
                    height: 46
                    radius: Theme.radius - 2
                    color: ma.containsMouse ? Theme.accent : Theme.surface

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        anchors.topMargin: 6
                        anchors.bottomMargin: 6
                        spacing: 2

                        Text {
                            text: modelData.name
                            font.family: Theme.fontFamily
                            font.pixelSize: 13
                            font.bold: true
                            color: ma.containsMouse ? Theme.textOnAccent : Theme.textPrimary
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Text {
                            text: modelData.comment
                            visible: text.length > 0
                            font.family: Theme.fontFamily
                            font.pixelSize: 11
                            color: ma.containsMouse ? Theme.textOnAccent : Theme.textSecondary
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    MouseArea {
                        id: ma
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            Launcher.launch(modelData.exec)
                            root.visible = false
                        }
                    }
                }
            }
        }
    }
}
