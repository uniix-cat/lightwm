set(SHELL_SOURCES
    main.cpp
    WmClient.hpp
    WmClient.cpp
    services/AppLauncherService.hpp
    services/AppLauncherService.cpp
    services/BackdropBlurService.hpp
    services/BackdropBlurService.cpp
    services/NetworkService.hpp
    services/NetworkService.cpp
    services/BluetoothService.hpp
    services/BluetoothService.cpp
    services/AudioService.hpp
    services/AudioService.cpp
    services/BatteryService.hpp
    services/BatteryService.cpp
    services/BrightnessService.hpp
    services/BrightnessService.cpp
    services/NotificationService.hpp
    services/NotificationService.cpp
    services/TrayService.hpp
    services/TrayService.cpp
    services/ScreenshotService.hpp
    services/ScreenshotService.cpp
    services/SessionLockIntegration.hpp
    services/SessionLockIntegration.cpp
    services/LockscreenService.hpp
    services/LockscreenService.cpp
)

qt_add_executable(lightwm-shell
    ${SHELL_SOURCES}
)

qt_add_qml_module(lightwm-shell
    URI LightWM.Shell
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/wallpaper/WallpaperWindow.qml
        qml/components/PillButton.qml
        qml/components/SliderRow.qml
        qml/components/ThemeSurface.qml
        qml/bar/BarWindow.qml
        qml/bar/WorkspacesWidget.qml
        qml/bar/TrayWidget.qml
        qml/bar/StatusCluster.qml
        qml/popovers/ControlCenterPopover.qml
        qml/launcher/LauncherOverlay.qml
        qml/notifications/NotificationStack.qml
        qml/notifications/OsdOverlay.qml
        qml/screenshot/ScreenshotOverlay.qml
        qml/lockscreen/LockscreenOverlay.qml
)

target_link_libraries(lightwm-shell PRIVATE
    lightwm_core
    lightwm_protocols
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::DBus
    Qt6::Network
    Qt6::WaylandClient
    Qt6::WaylandClientPrivate
    LayerShellQt::Interface
    PkgConfig::LIBPULSE
    PkgConfig::PAM
)

install(TARGETS lightwm-shell DESTINATION bin)
