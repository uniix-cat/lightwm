import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.layershell 1.0 as LayerShell
import "../components"

Window {
    id: root
    property string query: ""
    readonly property var results: Launcher.search(query)

    function launchIndex(idx) {
        if (idx >= 0 && idx < results.length) {
            Launcher.launch(results[idx].exec)
            root.visible = false
        }
    }

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
            appList.currentIndex = results.length > 0 ? 0 : -1
            appList.positionViewAtBeginning()
            searchField.forceActiveFocus()
        }
    }

    onResultsChanged: {
        appList.currentIndex = results.length > 0 ? 0 : -1
        if (results.length > 0) {
            appList.positionViewAtBeginning()
        }
    }

    ThemeSurface {
        anchors.fill: parent
        elevated: true
        radius: Theme.radius
        border.width: 2
        border.color: Theme.borderActive
        readonly property var activeScr: Qt.application.screens.length > 0 ? Qt.application.screens[0] : null
        backdropScreenName: activeScr ? activeScr.name : ""
        screenWidth: activeScr ? activeScr.width : 1920
        screenHeight: activeScr ? activeScr.height : 1080
        backdropRect: Qt.rect((screenWidth - root.width) / 2, (screenHeight - root.height) / 2, root.width, root.height)

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
                Keys.onDownPressed: (event) => {
                    if (appList.count > 0) {
                        appList.incrementCurrentIndex()
                        appList.positionViewAtIndex(appList.currentIndex, ListView.Contain)
                    }
                    event.accepted = true
                }
                Keys.onUpPressed: (event) => {
                    if (appList.count > 0) {
                        appList.decrementCurrentIndex()
                        appList.positionViewAtIndex(appList.currentIndex, ListView.Contain)
                    }
                    event.accepted = true
                }
                onAccepted: {
                    const idx = (appList.currentIndex >= 0 && appList.currentIndex < root.results.length)
                        ? appList.currentIndex
                        : 0
                    root.launchIndex(idx)
                }

                background: Rectangle {
                    radius: Theme.radius - 2
                    color: Theme.surface
                    border.width: 1
                    border.color: Theme.borderActive
                }
            }

            ListView {
                id: appList
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 6
                keyNavigationWraps: true
                currentIndex: root.results.length > 0 ? 0 : -1
                model: root.results
                delegate: Rectangle {
                    required property int index
                    required property var modelData
                    readonly property bool isSelected: ListView.isCurrentItem || ma.containsMouse
                    width: ListView.view.width
                    height: 46
                    radius: Theme.radius - 2
                    color: isSelected ? Theme.accent : Theme.surface
                    border.width: ListView.isCurrentItem ? 1 : 0
                    border.color: Theme.accentStrong

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
                            color: isSelected ? Theme.textOnAccent : Theme.textPrimary
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Text {
                            text: modelData.comment
                            visible: text.length > 0
                            font.family: Theme.fontFamily
                            font.pixelSize: 11
                            color: isSelected ? Theme.textOnAccent : Theme.textSecondary
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    MouseArea {
                        id: ma
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.launchIndex(index)
                    }
                }
            }
        }
    }
}
