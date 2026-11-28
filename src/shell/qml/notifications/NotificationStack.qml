import QtQuick
import QtQuick.Layouts
import org.kde.layershell 1.0 as LayerShell
import "../components"

Window {
    id: root
    readonly property int notifCount: Notifications.notifications.length
    onNotifCountChanged: {
        if (notifCount > 0 && !visible && Theme.blurEnabled && Theme.blurStrength > 0 && Backdrop.available) {
            Backdrop.capturePrimary()
        }
    }

    visible: notifCount > 0
    width: 340
    height: Math.min(480, Math.max(80, col.implicitHeight + 16))
    color: "transparent"

    LayerShell.Window.anchors: LayerShell.Window.AnchorTop | LayerShell.Window.AnchorRight
    LayerShell.Window.layer: LayerShell.Window.LayerOverlay
    LayerShell.Window.margins: Qt.margins(0, Theme.barHeight + 14, 12, 0)
    LayerShell.Window.keyboardInteractivity: LayerShell.Window.KeyboardInteractivityNone
    LayerShell.Window.scope: "lightwm-notifications"

    ColumnLayout {
        id: col
        anchors.fill: parent
        spacing: 8

        Repeater {
            model: Notifications.notifications
            delegate: ThemeSurface {
                id: toastCard
                required property var modelData
                Layout.fillWidth: true
                implicitHeight: innerCol.implicitHeight + 20
                elevated: true
                border.color: Theme.borderActive
                readonly property var activeScr: Qt.application.screens.length > 0 ? Qt.application.screens[0] : null
                backdropScreenName: activeScr ? activeScr.name : ""
                screenWidth: activeScr ? activeScr.width : 1920
                screenHeight: activeScr ? activeScr.height : 1080
                backdropRect: Qt.rect(screenWidth - root.width - 12, Theme.barHeight + 14 + toastCard.y, toastCard.width, toastCard.height)

                ColumnLayout {
                    id: innerCol
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 4

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: modelData.appName
                            font.family: Theme.fontFamily
                            font.pixelSize: 10
                            color: Theme.textSecondary
                            Layout.fillWidth: true
                        }
                        Text {
                            text: "×"
                            color: Theme.textSecondary
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: Notifications.dismiss(modelData.id)
                            }
                        }
                    }
                    Text {
                        text: modelData.summary
                        font.family: Theme.fontFamily
                        font.pixelSize: 13
                        font.bold: true
                        color: Theme.textPrimary
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    Text {
                        text: modelData.body
                        visible: text.length > 0
                        font.family: Theme.fontFamily
                        font.pixelSize: 12
                        color: Theme.textSecondary
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }
}
