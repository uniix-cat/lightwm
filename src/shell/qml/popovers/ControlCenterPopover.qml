import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.layershell 1.0 as LayerShell
import "../components"

Window {
    id: root
    property string pendingSsid: ""
    property bool showingPasswordPrompt: false

    visible: false
    width: 360
    height: 480
    color: "transparent"

    LayerShell.Window.anchors: LayerShell.Window.AnchorTop | LayerShell.Window.AnchorRight
    LayerShell.Window.layer: LayerShell.Window.LayerOverlay
    LayerShell.Window.margins: Qt.margins(0, Theme.barHeight + 14, 12, 0)
    LayerShell.Window.keyboardInteractivity: LayerShell.Window.KeyboardInteractivityOnDemand
    LayerShell.Window.scope: "lightwm-control-center"

    onVisibleChanged: {
        if (visible) {
            Network.refresh()
            Bluetooth.refresh()
            Audio.refresh()
            Brightness.refresh()
            Battery.refresh()
        }
    }

    ThemeSurface {
        anchors.fill: parent
        elevated: true
        radius: Theme.radius

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 12

            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: "Quick Settings"
                    font.family: Theme.fontFamily
                    font.pixelSize: 15
                    font.bold: true
                    color: Theme.textPrimary
                    Layout.fillWidth: true
                }
                PillButton {
                    text: "Settings"
                    onClicked: {
                        Launcher.launch("lightwm-settings")
                        root.visible = false
                    }
                }
            }

            SliderRow {
                label: Audio.muted ? "Muted" : "Volume"
                value: Audio.volume
                to: 100
                onMovedValue: (v) => Audio.setVolume(v)
            }

            SliderRow {
                visible: Brightness.available
                label: "Bright"
                value: Brightness.brightness
                from: 5
                to: 100
                onMovedValue: (v) => Brightness.setBrightness(v)
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                PillButton {
                    text: Network.wifiEnabled ? "Wi-Fi: ON" : "Wi-Fi: OFF"
                    active: Network.wifiEnabled
                    onClicked: Network.setWifiEnabled(!Network.wifiEnabled)
                }
                PillButton {
                    text: "Scan"
                    onClicked: Network.rescan()
                }
                PillButton {
                    text: Bluetooth.powered ? "BT: ON" : "BT: OFF"
                    active: Bluetooth.powered
                    onClicked: Bluetooth.setPowered(!Bluetooth.powered)
                }
            }

            // Inline Wi-Fi password prompt
            RowLayout {
                visible: root.showingPasswordPrompt
                Layout.fillWidth: true
                spacing: 6

                TextField {
                    id: wifiPassInput
                    Layout.fillWidth: true
                    placeholderText: "Password for " + root.pendingSsid
                    placeholderTextColor: Theme.textSecondary
                    echoMode: TextInput.Password
                    color: Theme.textPrimary
                    background: Rectangle {
                        radius: Theme.radius - 4
                        color: Theme.surface
                        border.width: 1
                        border.color: Theme.borderActive
                    }
                    onAccepted: {
                        Network.connectToNetwork(root.pendingSsid, text)
                        text = ""
                        root.showingPasswordPrompt = false
                    }
                }
                PillButton {
                    text: "Join"
                    active: true
                    onClicked: {
                        Network.connectToNetwork(root.pendingSsid, wifiPassInput.text)
                        wifiPassInput.text = ""
                        root.showingPasswordPrompt = false
                    }
                }
            }

            Text {
                text: Network.statusMessage.length > 0 ? ("Wi-Fi Networks — " + Network.statusMessage) : "Wi-Fi Networks"
                font.family: Theme.fontFamily
                font.pixelSize: 12
                font.bold: true
                color: Theme.textSecondary
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 4
                model: Network.networks
                delegate: Rectangle {
                    required property var modelData
                    width: ListView.view.width
                    height: 34
                    radius: 8
                    color: modelData.active ? Theme.accent : Theme.surface

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        spacing: 6
                        Text {
                            visible: modelData.secured
                            text: String.fromCodePoint(0xF033E)
                            font.family: Theme.iconFontFamily
                            font.pixelSize: 12
                            color: modelData.active ? Theme.textOnAccent : Theme.textSecondary
                        }
                        Text {
                            text: modelData.ssid + (modelData.saved && !modelData.active ? " (saved)" : "")
                            color: modelData.active ? Theme.textOnAccent : Theme.textPrimary
                            font.family: Theme.fontFamily
                            font.pixelSize: 12
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }
                        Text {
                            text: modelData.active ? "Connected" : (modelData.strength + "%")
                            color: modelData.active ? Theme.textOnAccent : Theme.textSecondary
                            font.family: Theme.fontFamily
                            font.pixelSize: 11
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (modelData.active) {
                                Network.disconnectWifi()
                            } else if (modelData.secured && !modelData.saved) {
                                root.pendingSsid = modelData.ssid
                                root.showingPasswordPrompt = true
                                wifiPassInput.forceActiveFocus()
                            } else {
                                Network.connectToNetwork(modelData.ssid)
                            }
                        }
                    }
                }
            }

            Text {
                text: "Bluetooth Devices"
                font.family: Theme.fontFamily
                font.pixelSize: 12
                font.bold: true
                color: Theme.textSecondary
                visible: Bluetooth.powered
            }

            ListView {
                visible: Bluetooth.powered
                Layout.fillWidth: true
                Layout.preferredHeight: 96
                clip: true
                spacing: 4
                model: Bluetooth.devices
                delegate: Rectangle {
                    required property var modelData
                    width: ListView.view.width
                    height: 30
                    radius: 8
                    color: modelData.connected ? Theme.accent : Theme.surface

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        Text {
                            text: modelData.name
                            color: modelData.connected ? Theme.textOnAccent : Theme.textPrimary
                            font.family: Theme.fontFamily
                            font.pixelSize: 12
                            Layout.fillWidth: true
                        }
                        Text {
                            text: modelData.connected ? "Connected" : "Pair/Connect"
                            color: modelData.connected ? Theme.textOnAccent : Theme.textSecondary
                            font.family: Theme.fontFamily
                            font.pixelSize: 11
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: Bluetooth.toggleDeviceConnection(modelData.path)
                    }
                }
            }
        }
    }
}
