import QtQuick
import QtQuick.Layouts
import "../components"

RowLayout {
    id: root
    signal controlCenterClicked()
    signal screenshotClicked()
    signal lockClicked()

    spacing: 6

    PillButton {
        text: Wm.isFloating ? "FLT" : "TILE"
        active: Wm.isFloating
        onClicked: Wm.toggleFloat()
    }

    PillButton {
        iconGlyph: String.fromCodePoint(0xF030C)
        text: Wm.keyboardLayout.slice(0, 2).toUpperCase()
    }

    PillButton {
        iconGlyph: String.fromCodePoint(0xF0100)
        onClicked: root.screenshotClicked()
    }

    // Brightness indicator (scroll wheel adjusts brightness)
    Item {
        visible: Brightness.available
        implicitWidth: brightPill.implicitWidth
        implicitHeight: brightPill.implicitHeight

        PillButton {
            id: brightPill
            iconGlyph: String.fromCodePoint(0xF00DF)
            text: Brightness.brightness + "%"
            onClicked: root.controlCenterClicked()
        }

        WheelHandler {
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            onWheel: (event) => {
                Brightness.stepBrightness(event.angleDelta.y > 0 ? 5 : -5)
            }
        }
    }

    // Volume indicator (click toggles mute, scroll wheel adjusts volume)
    Item {
        implicitWidth: volPill.implicitWidth
        implicitHeight: volPill.implicitHeight

        PillButton {
            id: volPill
            iconGlyph: Audio.muted ? String.fromCodePoint(0xF0581) : String.fromCodePoint(0xF057E)
            text: Audio.muted ? "Mute" : (Audio.volume + "%")
            active: Audio.muted
            onClicked: Audio.toggleMute()
        }

        WheelHandler {
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            onWheel: (event) => {
                Audio.stepVolume(event.angleDelta.y > 0 ? 5 : -5)
            }
        }
    }

    // Battery indicator (shown when battery is present)
    PillButton {
        visible: Battery.available
        iconGlyph: Battery.charging ? String.fromCodePoint(0xF0084) : String.fromCodePoint(0xF0079)
        text: Battery.percentage + "%"
        danger: !Battery.charging && Battery.percentage <= 15
        onClicked: root.controlCenterClicked()
    }

    // Wi-Fi + Quick Settings trigger
    PillButton {
        iconGlyph: Network.wifiEnabled ? String.fromCodePoint(0xF05A9) : String.fromCodePoint(0xF05AA)
        text: Network.activeSsid.length > 0 ? Network.activeSsid : (Network.wifiEnabled ? "Wi-Fi" : "Off")
        active: Network.activeSsid.length > 0
        onClicked: root.controlCenterClicked()
    }

    PillButton {
        iconGlyph: String.fromCodePoint(0xF0493)
        onClicked: Launcher.launch("lightwm-settings")
    }

    PillButton {
        iconGlyph: String.fromCodePoint(0xF0150)
        text: Qt.formatDateTime(clockTimer.now, "HH:mm")
        onClicked: root.lockClicked()
    }

    Timer {
        id: clockTimer
        property date now: new Date()
        interval: 15000
        running: true
        repeat: true
        onTriggered: now = new Date()
    }
}
