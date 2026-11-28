import QtQuick
import QtQuick.Effects

Rectangle {
    id: root
    property bool elevated: false
    property bool glass: true
    property string backdropScreenName: ""
    property rect backdropRect: Qt.rect(0, 0, width, height)
    property real screenWidth: 1920
    property real screenHeight: 1080
    property bool useWallpaperBackdrop: false

    readonly property bool hasLiveBackdrop: backdropScreenName.length > 0 && Backdrop.hasFrame(backdropScreenName)
    readonly property bool blurActive: glass && Theme.blurEnabled && Theme.blurStrength > 0
                                       && (hasLiveBackdrop || useWallpaperBackdrop)
    readonly property color baseColor: elevated ? Theme.surfaceElevated : Theme.surface
    readonly property color effectiveFill: (glass && Theme.blurEnabled)
        ? Qt.rgba(baseColor.r, baseColor.g, baseColor.b, Theme.glassOpacity)
        : baseColor

    color: blurActive ? "transparent" : effectiveFill
    radius: Theme.radius
    border.width: 1
    border.color: Theme.borderInactive

    Rectangle {
        id: maskRect
        z: -3
        anchors.fill: parent
        radius: root.radius
        color: "white"
        visible: false
        layer.enabled: true
    }

    Item {
        id: backdropLayer
        z: -3
        anchors.fill: parent
        clip: true
        visible: false
        layer.enabled: true

        // Live captured desktop frame behind floating overlays
        Image {
            visible: root.hasLiveBackdrop && !root.useWallpaperBackdrop
            x: -root.backdropRect.x
            y: -root.backdropRect.y
            width: root.screenWidth
            height: root.screenHeight
            source: (root.hasLiveBackdrop && !root.useWallpaperBackdrop)
                ? ("image://backdrop/" + root.backdropScreenName + "?rev=" + Backdrop.revision)
                : ""
            fillMode: Image.Stretch
            smooth: true
            cache: false
        }

        // Wallpaper / theme gradient fallback (used by top bar & when live capture isn't ready)
        Item {
            visible: !root.hasLiveBackdrop || root.useWallpaperBackdrop
            x: -root.backdropRect.x
            y: -root.backdropRect.y
            width: root.screenWidth
            height: root.screenHeight

            Rectangle {
                anchors.fill: parent
                visible: Theme.wallpaperPath.length === 0
                gradient: Gradient {
                    orientation: Gradient.Vertical
                    GradientStop { position: 0.0; color: Theme.background }
                    GradientStop { position: 0.72; color: Theme.surface }
                    GradientStop { position: 1.0; color: Qt.tint(Theme.surface, Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.25)) }
                }
            }

            Image {
                anchors.fill: parent
                visible: Theme.wallpaperPath.length > 0
                source: Theme.wallpaperPath.length > 0 ? ("file://" + Theme.wallpaperPath) : ""
                asynchronous: true
                smooth: true
                fillMode: Theme.wallpaperMode === "fit" ? Image.PreserveAspectFit
                        : Theme.wallpaperMode === "stretch" ? Image.Stretch
                        : Theme.wallpaperMode === "center" ? Image.Pad
                        : Image.PreserveAspectCrop
            }
        }
    }

    MultiEffect {
        id: blurEffect
        z: -2
        anchors.fill: parent
        visible: root.blurActive
        source: backdropLayer
        autoPaddingEnabled: false
        blurEnabled: true
        blur: 1.0
        blurMax: Theme.blurStrength
        blurMultiplier: 1.45
        saturation: Theme.isDark ? 0.16 : 0.08
        brightness: Theme.isDark ? -0.05 : 0.03
        maskEnabled: true
        maskSource: maskRect
    }

    // Translucent frosted glass tint + specular inner edge
    Rectangle {
        z: -1
        anchors.fill: parent
        visible: root.blurActive
        radius: root.radius
        color: root.effectiveFill
        border.width: 1
        border.color: Theme.isDark ? "#24FFFFFF" : "#66FFFFFF"
    }
}
