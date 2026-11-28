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
    readonly property var tabs: ["Appearance", "Qt & GTK", "Tiling & Gaps", "Input", "Displays", "Shortcuts"]

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
                ScrollView {
                    id: appearanceScroll
                    clip: true

                    ColumnLayout {
                        width: appearanceScroll.availableWidth
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
                            text: "Frosted Glass (Gaussian Blur)"
                            font.family: Theme.fontFamily
                            font.pixelSize: 15
                            font.bold: true
                            color: Theme.textPrimary
                            Layout.topMargin: 4
                        }

                        CheckBox {
                            text: "Enable Gaussian Blur on Shell Panels & Overlays"
                            checked: Settings.blurEnabled
                            onToggled: Settings.blurEnabled = checked
                        }

                        RowLayout {
                            spacing: 12
                            enabled: Settings.blurEnabled
                            Text { text: "Blur Radius: " + Settings.blurStrength + " px"; color: Theme.textPrimary; Layout.preferredWidth: 160 }
                            Slider {
                                from: 8; to: 64; stepSize: 2; value: Settings.blurStrength
                                onMoved: Settings.blurStrength = Math.round(value)
                            }
                        }

                        RowLayout {
                            spacing: 12
                            enabled: Settings.blurEnabled
                            Text { text: "Glass Opacity: " + Math.round(Settings.glassOpacity * 100) + "%"; color: Theme.textPrimary; Layout.preferredWidth: 160 }
                            Slider {
                                from: 0.35; to: 0.95; stepSize: 0.05; value: Settings.glassOpacity
                                onMoved: Settings.glassOpacity = Math.round(value * 20) / 20
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
                    }
                }

                // 1: Qt & GTK Toolkit Themes
                ScrollView {
                    id: toolkitScroll
                    clip: true

                    ColumnLayout {
                        width: toolkitScroll.availableWidth
                        spacing: 14

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: "Qt & GTK Themes"
                                font.family: Theme.fontFamily
                                font.pixelSize: 18
                                font.bold: true
                                color: Theme.textPrimary
                                Layout.fillWidth: true
                            }
                            Button {
                                text: "Refresh Lists"
                                onClicked: Settings.refreshToolkitThemes()
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12
                            Text {
                                text: "Color Scheme:"
                                font.family: Theme.fontFamily
                                color: Theme.textPrimary
                                Layout.preferredWidth: 150
                            }
                            Repeater {
                                model: [
                                    { id: "prefer-dark", label: "Prefer Dark" },
                                    { id: "prefer-light", label: "Prefer Light" },
                                    { id: "default", label: "Default" }
                                ]
                                delegate: Rectangle {
                                    required property var modelData
                                    readonly property bool isCurrent: Settings.colorScheme === modelData.id
                                    width: schemeText.implicitWidth + 20
                                    height: 30
                                    radius: Theme.radius - 3
                                    color: isCurrent ? Theme.accent : Theme.surfaceElevated
                                    border.width: 1
                                    border.color: isCurrent ? Theme.accentStrong : Theme.borderInactive

                                    Text {
                                        id: schemeText
                                        anchors.centerIn: parent
                                        text: modelData.label
                                        font.family: Theme.fontFamily
                                        font.pixelSize: 12
                                        font.weight: isCurrent ? Font.DemiBold : Font.Normal
                                        color: isCurrent ? Theme.textOnAccent : Theme.textPrimary
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: Settings.colorScheme = modelData.id
                                    }
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12
                            Text {
                                text: "GTK Theme:"
                                font.family: Theme.fontFamily
                                color: Theme.textPrimary
                                Layout.preferredWidth: 150
                            }
                            ComboBox {
                                Layout.fillWidth: true
                                model: Settings.availableGtkThemes
                                currentIndex: Math.max(0, model.indexOf(Settings.gtkTheme))
                                enabled: model.length > 0
                                onActivated: Settings.gtkTheme = currentText
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12
                            Text {
                                text: "Qt Platform Theme:"
                                font.family: Theme.fontFamily
                                color: Theme.textPrimary
                                Layout.preferredWidth: 150
                            }
                            ComboBox {
                                Layout.fillWidth: true
                                model: Settings.availableQtPlatformThemes
                                currentIndex: Math.max(0, model.indexOf(Settings.qtPlatformTheme))
                                onActivated: Settings.qtPlatformTheme = currentText
                            }
                        }

                        Text {
                            text: "Resolved Qt backend: " + (Settings.resolvedQtPlatformTheme.length > 0 ? Settings.resolvedQtPlatformTheme : "none")
                                  + "  (auto follows GTK3 theme/dark mode, or choose qt6ct / kde)"
                            font.family: Theme.fontFamily
                            font.pixelSize: 11
                            color: Theme.textSecondary
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12
                            Text {
                                text: "Qt Widget Style:"
                                font.family: Theme.fontFamily
                                color: Theme.textPrimary
                                Layout.preferredWidth: 150
                            }
                            ComboBox {
                                Layout.fillWidth: true
                                model: Settings.availableQtStyles
                                displayText: (currentIndex >= 0 && model[currentIndex].length > 0)
                                    ? model[currentIndex]
                                    : "(Platform Default)"
                                currentIndex: Math.max(0, model.indexOf(Settings.qtStyle))
                                delegate: ItemDelegate {
                                    required property var modelData
                                    required property int index
                                    width: ListView.view.width
                                    text: modelData.length > 0 ? modelData : "(Platform Default)"
                                    highlighted: ListView.isCurrentItem
                                }
                                onActivated: (idx) => Settings.qtStyle = model[idx]
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12
                            Text {
                                text: "Icon Theme:"
                                font.family: Theme.fontFamily
                                color: Theme.textPrimary
                                Layout.preferredWidth: 150
                            }
                            ComboBox {
                                Layout.fillWidth: true
                                model: Settings.availableIconThemes
                                currentIndex: Math.max(0, model.indexOf(Settings.iconTheme))
                                enabled: model.length > 0
                                onActivated: Settings.iconTheme = currentText
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12
                            Text {
                                text: "Cursor Theme:"
                                font.family: Theme.fontFamily
                                color: Theme.textPrimary
                                Layout.preferredWidth: 150
                            }
                            ComboBox {
                                Layout.fillWidth: true
                                model: Settings.availableCursorThemes
                                currentIndex: Math.max(0, model.indexOf(Settings.cursorTheme))
                                enabled: model.length > 0
                                onActivated: Settings.cursorTheme = currentText
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12
                            Text {
                                text: "Cursor Size: " + Settings.cursorSize + " px"
                                font.family: Theme.fontFamily
                                color: Theme.textPrimary
                                Layout.preferredWidth: 150
                            }
                            Slider {
                                id: cursorSlider
                                Layout.fillWidth: true
                                from: 16
                                to: 48
                                stepSize: 2
                                value: Settings.cursorSize
                                onMoved: {
                                    if (!pressed) {
                                        Settings.cursorSize = Math.round(value)
                                    }
                                }
                                onPressedChanged: {
                                    if (!pressed) {
                                        Settings.cursorSize = Math.round(value)
                                    }
                                }
                            }
                        }

                        Text {
                            text: "GTK3/GTK4 themes, icons and color scheme apply live via gsettings & settings.ini. Newly launched Qt apps use QT_QPA_PLATFORMTHEME and QT_STYLE_OVERRIDE immediately."
                            font.family: Theme.fontFamily
                            font.pixelSize: 11
                            color: Theme.textSecondary
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                            Layout.topMargin: 4
                        }
                    }
                }

                // 2: Tiling & Gaps
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

                // 3: Displays & Fractional Scaling
                ColumnLayout {
                    spacing: 12
                    Text {
                        text: "Displays & Fractional Scaling"
                        font.family: Theme.fontFamily
                        font.pixelSize: 18
                        font.bold: true
                        color: Theme.textPrimary
                    }
                    ScrollView {
                        id: displaysScroll
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true

                        ColumnLayout {
                            width: displaysScroll.availableWidth
                            spacing: 12

                            Repeater {
                                model: Settings.displays
                                delegate: Rectangle {
                                    id: displayCard
                                    required property var modelData
                                    property double liveScale: modelData.configuredScale !== undefined ? modelData.configuredScale : modelData.scale
                                    property double pendingScale: liveScale
                                    onLiveScaleChanged: {
                                        if (!scaleSlider.pressed) {
                                            pendingScale = liveScale
                                        }
                                    }

                                    Layout.fillWidth: true
                                    implicitHeight: cardLayout.implicitHeight + 24
                                    radius: Theme.radius - 2
                                    color: Theme.surfaceElevated
                                    border.width: 1
                                    border.color: Theme.borderInactive

                                    ColumnLayout {
                                        id: cardLayout
                                        anchors.fill: parent
                                        anchors.margins: 12
                                        spacing: 10

                                        RowLayout {
                                            Layout.fillWidth: true
                                            Text {
                                                text: displayCard.modelData.name + " — " + displayCard.modelData.width + "×" + displayCard.modelData.height + " @ " + displayCard.modelData.refresh + " Hz"
                                                font.family: Theme.fontFamily
                                                font.bold: true
                                                font.pixelSize: 14
                                                color: Theme.textPrimary
                                                Layout.fillWidth: true
                                            }
                                            Text {
                                                text: Math.round(displayCard.pendingScale * 100) + "% (" + displayCard.pendingScale.toFixed(2) + "x)"
                                                font.family: Theme.fontFamily
                                                font.bold: true
                                                font.pixelSize: 13
                                                color: Theme.accentStrong
                                            }
                                        }

                                        Text {
                                            text: (displayCard.modelData.description && displayCard.modelData.description.length > 0
                                                   ? displayCard.modelData.description + "  •  "
                                                   : "")
                                                  + "Logical: " + displayCard.modelData.logicalWidth + "×" + displayCard.modelData.logicalHeight
                                                  + "  •  Active: " + Number(displayCard.modelData.scale).toFixed(2) + "x"
                                            font.family: Theme.fontFamily
                                            font.pixelSize: 11
                                            color: Theme.textSecondary
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }

                                        RowLayout {
                                            Layout.fillWidth: true
                                            spacing: 6

                                            Text {
                                                text: "Presets:"
                                                font.family: Theme.fontFamily
                                                font.pixelSize: 12
                                                color: Theme.textSecondary
                                            }

                                            Repeater {
                                                model: [0.75, 1.0, 1.25, 1.5, 1.75, 2.0]
                                                delegate: Rectangle {
                                                    required property double modelData
                                                    readonly property bool isCurrent: Math.abs(displayCard.liveScale - modelData) < 0.01
                                                    width: presetLabel.implicitWidth + 16
                                                    height: 26
                                                    radius: Theme.radius - 4
                                                    color: isCurrent ? Theme.accent : Theme.surface
                                                    border.width: 1
                                                    border.color: isCurrent ? Theme.accentStrong : Theme.borderInactive

                                                    Text {
                                                        id: presetLabel
                                                        anchors.centerIn: parent
                                                        text: Math.round(modelData * 100) + "%"
                                                        font.family: Theme.fontFamily
                                                        font.pixelSize: 11
                                                        font.weight: isCurrent ? Font.DemiBold : Font.Normal
                                                        color: isCurrent ? Theme.textOnAccent : Theme.textPrimary
                                                    }

                                                    MouseArea {
                                                        anchors.fill: parent
                                                        cursorShape: Qt.PointingHandCursor
                                                        onClicked: {
                                                            displayCard.pendingScale = modelData
                                                            Settings.setOutputScale(displayCard.modelData.name, modelData)
                                                        }
                                                    }
                                                }
                                            }
                                        }

                                        RowLayout {
                                            Layout.fillWidth: true
                                            spacing: 12

                                            Text {
                                                text: "Custom Scale:"
                                                font.family: Theme.fontFamily
                                                font.pixelSize: 12
                                                color: Theme.textPrimary
                                            }

                                            Slider {
                                                id: scaleSlider
                                                Layout.fillWidth: true
                                                from: 0.5
                                                to: 3.0
                                                stepSize: 0.05
                                                value: displayCard.liveScale
                                                onMoved: {
                                                    displayCard.pendingScale = Math.round(value * 20) / 20
                                                    if (!pressed) {
                                                        Settings.setOutputScale(displayCard.modelData.name, displayCard.pendingScale)
                                                    }
                                                }
                                                onPressedChanged: {
                                                    if (!pressed) {
                                                        const target = Math.round(value * 20) / 20
                                                        displayCard.pendingScale = target
                                                        if (Math.abs(target - displayCard.liveScale) >= 0.01) {
                                                            Settings.setOutputScale(displayCard.modelData.name, target)
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // 5: Shortcuts & Default Terminal
                ColumnLayout {
                    id: shortcutsTab
                    property int recordingIndex: -1
                    property bool addingCustom: false
                    property string customMods: "Super"
                    property string customKey: ""
                    property bool recordingCustom: false
                    spacing: 12

                    Text {
                        text: "Default Terminal & Shortcuts"
                        font.family: Theme.fontFamily
                        font.pixelSize: 18
                        font.bold: true
                        color: Theme.textPrimary
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: termRow.implicitHeight + 16
                        radius: Theme.radius - 2
                        color: Theme.surfaceElevated
                        border.width: 1
                        border.color: Theme.borderInactive

                        RowLayout {
                            id: termRow
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 10

                            Text {
                                text: "Default Terminal:"
                                font.family: Theme.fontFamily
                                font.bold: true
                                color: Theme.textPrimary
                            }

                            ComboBox {
                                Layout.preferredWidth: 160
                                model: Settings.availableTerminals
                                currentIndex: Math.max(0, model.indexOf(Settings.terminal))
                                onActivated: Settings.terminal = currentText
                            }

                            TextField {
                                Layout.fillWidth: true
                                text: Settings.terminal
                                placeholderText: "Custom terminal command..."
                                color: Theme.textPrimary
                                onEditingFinished: {
                                    if (text.trim().length > 0) {
                                        Settings.terminal = text.trim()
                                    }
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text {
                            text: "Keybindings (click a shortcut pill to rebind)"
                            font.family: Theme.fontFamily
                            font.pixelSize: 13
                            font.bold: true
                            color: Theme.textSecondary
                            Layout.fillWidth: true
                        }
                        Button {
                            text: shortcutsTab.addingCustom ? "Close" : "+ Add Custom"
                            onClicked: {
                                shortcutsTab.addingCustom = !shortcutsTab.addingCustom
                                shortcutsTab.recordingCustom = false
                            }
                        }
                        Button {
                            text: "Reset Defaults"
                            onClicked: {
                                shortcutsTab.recordingIndex = -1
                                Settings.resetShortcuts()
                            }
                        }
                    }

                    Rectangle {
                        visible: shortcutsTab.addingCustom
                        Layout.fillWidth: true
                        implicitHeight: addRow.implicitHeight + 16
                        radius: Theme.radius - 2
                        color: Theme.surfaceElevated
                        border.width: 1
                        border.color: Theme.borderActive

                        RowLayout {
                            id: addRow
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 8

                            TextField {
                                id: customCmdField
                                Layout.fillWidth: true
                                placeholderText: "Shell command (e.g. firefox, thunar)"
                                color: Theme.textPrimary
                            }

                            Rectangle {
                                width: customKeyText.implicitWidth + 20
                                height: 30
                                radius: Theme.radius - 4
                                color: shortcutsTab.recordingCustom ? Theme.accent : Theme.surface
                                border.width: 1
                                border.color: shortcutsTab.recordingCustom ? Theme.accentStrong : Theme.borderInactive

                                Text {
                                    id: customKeyText
                                    anchors.centerIn: parent
                                    text: shortcutsTab.recordingCustom
                                          ? "Press keys..."
                                          : (shortcutsTab.customKey.length > 0
                                             ? (shortcutsTab.customMods.length > 0 ? shortcutsTab.customMods + " + " : "") + shortcutsTab.customKey
                                             : "Bind Keys...")
                                    font.family: Theme.fontFamily
                                    font.pixelSize: 12
                                    color: shortcutsTab.recordingCustom ? Theme.textOnAccent : Theme.textPrimary
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        shortcutsTab.recordingIndex = -1
                                        shortcutsTab.recordingCustom = true
                                        customKeyRecorder.forceActiveFocus()
                                    }
                                }

                                Item {
                                    id: customKeyRecorder
                                    Keys.onPressed: (event) => {
                                        if (!shortcutsTab.recordingCustom) return
                                        if (event.key === Qt.Key_Escape && event.modifiers === Qt.NoModifier) {
                                            shortcutsTab.recordingCustom = false
                                            event.accepted = true
                                            return
                                        }
                                        const k = Settings.qtKeyToXkbName(event.key)
                                        if (k.length > 0) {
                                            shortcutsTab.customMods = Settings.formatModifiers(event.modifiers)
                                            shortcutsTab.customKey = k
                                            shortcutsTab.recordingCustom = false
                                        }
                                        event.accepted = true
                                    }
                                }
                            }

                            Button {
                                text: "Add"
                                enabled: customCmdField.text.trim().length > 0 && shortcutsTab.customKey.length > 0
                                onClicked: {
                                    Settings.addCustomShortcut(shortcutsTab.customMods, shortcutsTab.customKey, customCmdField.text)
                                    customCmdField.text = ""
                                    shortcutsTab.customKey = ""
                                    shortcutsTab.addingCustom = false
                                }
                            }
                        }
                    }

                    ScrollView {
                        id: shortcutsScroll
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true

                        ColumnLayout {
                            width: shortcutsScroll.availableWidth
                            spacing: 6

                            Repeater {
                                model: Settings.shortcuts
                                delegate: Rectangle {
                                    id: scRow
                                    required property var modelData
                                    readonly property bool isRecording: shortcutsTab.recordingIndex === modelData.index
                                    Layout.fillWidth: true
                                    height: 40
                                    radius: Theme.radius - 3
                                    color: Theme.surfaceElevated
                                    border.width: 1
                                    border.color: isRecording ? Theme.accentStrong : Theme.borderInactive

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 12
                                        anchors.rightMargin: 10
                                        spacing: 8

                                        Text {
                                            text: scRow.modelData.label
                                            font.family: Theme.fontFamily
                                            font.pixelSize: 13
                                            color: Theme.textPrimary
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }

                                        Rectangle {
                                            width: Math.max(96, keyPillText.implicitWidth + 20)
                                            height: 26
                                            radius: Theme.radius - 4
                                            color: scRow.isRecording ? Theme.accent : Theme.surface
                                            border.width: 1
                                            border.color: scRow.isRecording ? Theme.accentStrong : Theme.borderInactive

                                            Text {
                                                id: keyPillText
                                                anchors.centerIn: parent
                                                text: scRow.isRecording ? "Press keys... (Esc)" : scRow.modelData.displayCombo
                                                font.family: Theme.fontFamily
                                                font.pixelSize: 11
                                                font.bold: true
                                                color: scRow.isRecording ? Theme.textOnAccent : Theme.textPrimary
                                            }

                                            MouseArea {
                                                anchors.fill: parent
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: {
                                                    shortcutsTab.recordingCustom = false
                                                    shortcutsTab.recordingIndex = scRow.modelData.index
                                                    keyRecorder.forceActiveFocus()
                                                }
                                            }

                                            Item {
                                                id: keyRecorder
                                                Keys.onPressed: (event) => {
                                                    if (!scRow.isRecording) return
                                                    if (event.key === Qt.Key_Escape && event.modifiers === Qt.NoModifier) {
                                                        shortcutsTab.recordingIndex = -1
                                                        event.accepted = true
                                                        return
                                                    }
                                                    const k = Settings.qtKeyToXkbName(event.key)
                                                    if (k.length > 0) {
                                                        const mods = Settings.formatModifiers(event.modifiers)
                                                        Settings.updateShortcut(scRow.modelData.index, mods, k)
                                                        shortcutsTab.recordingIndex = -1
                                                    }
                                                    event.accepted = true
                                                }
                                            }
                                        }

                                        Button {
                                            visible: scRow.modelData.isCustom
                                            text: "×"
                                            implicitWidth: 28
                                            implicitHeight: 26
                                            onClicked: Settings.removeShortcut(scRow.modelData.index)
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Text {
                        text: "Fixed bindings: Super + 1..9 / Shift + 1..9 (workspaces)  |  Super + LMB/RMB Drag (move/resize)"
                        font.family: Theme.fontFamily
                        font.pixelSize: 11
                        color: Theme.textSecondary
                    }
                }
            }
        }
    }
}
