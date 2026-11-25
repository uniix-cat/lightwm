import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

ApplicationWindow {
    id: win
    visible: true
    width: 820
    height: 540
    minimumWidth: 680
    minimumHeight: 460
    title: "LightWM Settings"
    color: Theme.background

    palette.window: Theme.background
    palette.windowText: Theme.textPrimary
    palette.base: Theme.surfaceElevated
    palette.alternateBase: Theme.surface
    palette.text: Theme.textPrimary
    palette.placeholderText: Theme.textSecondary
    palette.button: Theme.surfaceElevated
    palette.buttonText: Theme.textPrimary
    palette.highlight: Theme.accent
    palette.highlightedText: Theme.textOnAccent
    palette.mid: Theme.borderInactive

    property int currentTab: 0
    readonly property var tabs: ["Appearance", "Tiling & Gaps", "Input", "Displays", "Shortcuts"]

    RowLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 16

        // Sidebar
        Rectangle {
            Layout.preferredWidth: 190
            Layout.fillHeight: true
            radius: Theme.radius
            color: Theme.surface
            border.width: 1
            border.color: Theme.borderInactive

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 6

                Text {
                    text: "LightWM"
                    font.family: Theme.fontFamily
                    font.pixelSize: 18
                    font.bold: true
                    color: Theme.textPrimary
                    Layout.bottomMargin: 8
                }

                Repeater {
                    model: win.tabs
                    delegate: Rectangle {
                        required property int index
                        required property string modelData
                        Layout.fillWidth: true
                        height: 36
                        radius: Theme.radius - 2
                        color: win.currentTab === index ? Theme.accent : "transparent"

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 12
                            text: modelData
                            font.family: Theme.fontFamily
                            font.pixelSize: 13
                            font.weight: win.currentTab === index ? Font.DemiBold : Font.Normal
                            color: win.currentTab === index ? Theme.textOnAccent : Theme.textPrimary
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: win.currentTab = index
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }

        // Content area
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: Theme.radius
            color: Theme.surface
            border.width: 1
            border.color: Theme.borderInactive

            StackLayout {
                anchors.fill: parent
                anchors.margins: 20
                currentIndex: win.currentTab

                // 0: Appearance & Themes
                ColumnLayout {
                    spacing: 16
                    Text {
                        text: "Appearance & Theme"
                        font.family: Theme.fontFamily
                        font.pixelSize: 18
                        font.bold: true
                        color: Theme.textPrimary
                    }

                    RowLayout {
                        spacing: 12
                        Repeater {
                            model: [
                                { id: "pine", title: "Slate & Pine (Dark)", bg: "#1e2224", accent: "#2d6a4f" },
                                { id: "cream", title: "Cream & Butter (Light)", bg: "#fffdd0", accent: "#ffec8b" }
                            ]
                            delegate: Rectangle {
                                required property var modelData
                                width: 210
                                height: 96
                                radius: Theme.radius
                                color: modelData.bg
                                border.width: Settings.themeName === modelData.id ? 3 : 1
                                border.color: Settings.themeName === modelData.id ? Theme.accentStrong : Theme.borderInactive

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 12
                                    Rectangle {
                                        width: 44
                                        height: 18
                                        radius: 9
                                        color: modelData.accent
                                    }
                                    Text {
                                        text: modelData.title
                                        font.family: Theme.fontFamily
                                        font.pixelSize: 13
                                        font.bold: true
                                        color: modelData.id === "cream" ? "#2a2618" : "#eceff1"
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: Settings.themeName = modelData.id
                                }
                            }
                        }
                    }

                    RowLayout {
                        spacing: 12
                        Text { text: "Corner Radius: " + Settings.cornerRadius + "px"; color: Theme.textPrimary; Layout.preferredWidth: 160 }
                        Slider {
                            from: 0; to: 20; stepSize: 1; value: Settings.cornerRadius
                            onMoved: Settings.cornerRadius = Math.round(value)
                        }
                    }

                    Text {
                        text: "Wallpaper"
                        font.family: Theme.fontFamily
                        font.pixelSize: 15
                        font.bold: true
                        color: Theme.textPrimary
                        Layout.topMargin: 6
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        Rectangle {
                            width: 144
                            height: 82
                            radius: Theme.radius - 2
                            clip: true
                            border.width: 1
                            border.color: Theme.borderActive
                            gradient: Gradient {
                                GradientStop { position: 0.0; color: Theme.background }
                                GradientStop { position: 1.0; color: Theme.accent }
                            }

                            Image {
                                anchors.fill: parent
                                visible: Settings.wallpaperPath.length > 0
                                source: Settings.wallpaperPath.length > 0 ? "file://" + Settings.wallpaperPath : ""
                                fillMode: Settings.wallpaperMode === "fit" ? Image.PreserveAspectFit
                                        : Settings.wallpaperMode === "stretch" ? Image.Stretch
                                        : Settings.wallpaperMode === "center" ? Image.Pad
                                        : Image.PreserveAspectCrop
                                asynchronous: true
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                TextField {
                                    Layout.fillWidth: true
                                    text: Settings.wallpaperPath
                                    placeholderText: "Built-in theme gradient (no image selected)"
                                    color: Theme.textPrimary
                                    onEditingFinished: Settings.wallpaperPath = text
                                }
                                Button {
                                    text: "Browse..."
                                    onClicked: wallpaperDialog.open()
                                }
                                Button {
                                    text: "Reset"
                                    enabled: Settings.wallpaperPath.length > 0
                                    onClicked: Settings.wallpaperPath = ""
                                }
                            }

                            RowLayout {
                                spacing: 8
                                Text { text: "Fill Mode:"; color: Theme.textSecondary }
                                ComboBox {
                                    model: ["fill", "fit", "stretch", "center"]
                                    currentIndex: Math.max(0, model.indexOf(Settings.wallpaperMode))
                                    onActivated: Settings.wallpaperMode = currentText
                                }
                            }
                        }
                    }

                    FileDialog {
                        id: wallpaperDialog
                        title: "Choose Wallpaper"
                        nameFilters: ["Images (*.png *.jpg *.jpeg *.webp *.bmp *.svg)"]
                        onAccepted: Settings.setWallpaperFromUrl(selectedFile)
                    }

                    Item { Layout.fillHeight: true }
                }

                // 1: Tiling & Gaps
                ColumnLayout {
                    spacing: 14
                    Text {
                        text: "Dwindle Tiling & Gaps"
                        font.family: Theme.fontFamily
                        font.pixelSize: 18
                        font.bold: true
                        color: Theme.textPrimary
                    }
                    RowLayout {
                        Text { text: "Inner Gaps: " + Settings.gapsInner + "px"; color: Theme.textPrimary; Layout.preferredWidth: 160 }
                        Slider { from: 0; to: 32; stepSize: 1; value: Settings.gapsInner; onMoved: Settings.gapsInner = Math.round(value) }
                    }
                    RowLayout {
                        Text { text: "Outer Gaps: " + Settings.gapsOuter + "px"; color: Theme.textPrimary; Layout.preferredWidth: 160 }
                        Slider { from: 0; to: 40; stepSize: 1; value: Settings.gapsOuter; onMoved: Settings.gapsOuter = Math.round(value) }
                    }
                    RowLayout {
                        Text { text: "Border Width: " + Settings.borderWidth + "px"; color: Theme.textPrimary; Layout.preferredWidth: 160 }
                        Slider { from: 0; to: 8; stepSize: 1; value: Settings.borderWidth; onMoved: Settings.borderWidth = Math.round(value) }
                    }
                    RowLayout {
                        Text { text: "Workspaces: " + Settings.workspaceCount; color: Theme.textPrimary; Layout.preferredWidth: 160 }
                        Slider { from: 3; to: 12; stepSize: 1; value: Settings.workspaceCount; onMoved: Settings.workspaceCount = Math.round(value) }
                    }
                    Item { Layout.fillHeight: true }
                }

                // 2: Input & Keyboard
                ColumnLayout {
                    spacing: 14
                    Text {
                        text: "Keyboard & Touchpad"
                        font.family: Theme.fontFamily
                        font.pixelSize: 18
                        font.bold: true
                        color: Theme.textPrimary
                    }
                    RowLayout {
                        spacing: 10
                        Text { text: "Layouts:"; color: Theme.textPrimary; Layout.preferredWidth: 130 }
                        TextField {
                            text: Settings.keyboardLayouts
                            color: Theme.textPrimary
                            onEditingFinished: Settings.keyboardLayouts = text
                        }
                    }
                    RowLayout {
                        spacing: 10
                        Text { text: "XKB Options:"; color: Theme.textPrimary; Layout.preferredWidth: 130 }
                        TextField {
                            text: Settings.keyboardOptions
                            color: Theme.textPrimary
                            onEditingFinished: Settings.keyboardOptions = text
                        }
                    }
                    CheckBox {
                        text: "Touchpad Tap-to-Click"
                        checked: Settings.tapToClick
                        onToggled: Settings.tapToClick = checked
                    }
                    CheckBox {
                        text: "Natural Scrolling"
                        checked: Settings.naturalScroll
                        onToggled: Settings.naturalScroll = checked
                    }
                    RowLayout {
                        spacing: 12
                        Text {
                            text: "Pointer / Touchpad Speed: " + Settings.pointerSpeed.toFixed(2)
                            color: Theme.textPrimary
                            Layout.preferredWidth: 220
                        }
                        Slider {
                            from: -1.0
                            to: 1.0
                            stepSize: 0.05
                            value: Settings.pointerSpeed
                            onMoved: Settings.pointerSpeed = Math.round(value * 20) / 20
                        }
                    }
                    Item { Layout.fillHeight: true }
                }

                // 3: Displays (multi-monitor foundation)
                ColumnLayout {
                    spacing: 12
                    Text {
                        text: "Displays (Multi-Monitor Foundation)"
                        font.family: Theme.fontFamily
                        font.pixelSize: 18
                        font.bold: true
                        color: Theme.textPrimary
                    }
                    Repeater {
                        model: Settings.displays
                        delegate: Rectangle {
                            required property var modelData
                            Layout.fillWidth: true
                            height: 56
                            radius: Theme.radius - 2
                            color: Theme.surfaceElevated
                            border.width: 1
                            border.color: Theme.borderInactive

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 10
                                Text {
                                    text: modelData.name + " — " + modelData.width + "x" + modelData.height + " @ " + modelData.refresh + " Hz"
                                    font.bold: true
                                    color: Theme.textPrimary
                                }
                                Text {
                                    text: "Scale: " + modelData.scale + "x (wlr-output-management config UI planned)"
                                    font.pixelSize: 11
                                    color: Theme.textSecondary
                                }
                            }
                        }
                    }
                    Item { Layout.fillHeight: true }
                }

                // 4: Default Keybindings Reference
                ColumnLayout {
                    spacing: 8
                    Text {
                        text: "Default Shortcuts"
                        font.family: Theme.fontFamily
                        font.pixelSize: 18
                        font.bold: true
                        color: Theme.textPrimary
                    }
                    Text { text: "Super + Q — Open Terminal"; color: Theme.textPrimary }
                    Text { text: "Super + R — Toggle App Launcher"; color: Theme.textPrimary }
                    Text { text: "Super + C — Close Focused Window"; color: Theme.textPrimary }
                    Text { text: "Super + V — Toggle Floating Mode"; color: Theme.textPrimary }
                    Text { text: "Super + F — Toggle Fullscreen"; color: Theme.textPrimary }
                    Text { text: "Super + LMB Drag — Move Floating Window"; color: Theme.textPrimary }
                    Text { text: "Super + RMB Drag — Resize Floating Window"; color: Theme.textPrimary }
                    Text { text: "Super + H/J/K/L — Move Focus (Left/Down/Up/Right)"; color: Theme.textPrimary }
                    Text { text: "Super + 1..9 / Shift+1..9 — Switch / Move Workspace"; color: Theme.textPrimary }
                    Text { text: "Super + Shift + L — Lock Screen   |   Print — Screenshot"; color: Theme.textPrimary }
                    Item { Layout.fillHeight: true }
                }
            }
        }
    }
}
