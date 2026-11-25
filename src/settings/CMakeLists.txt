qt_add_executable(lightwm-settings
    main.cpp
    SettingsBackend.hpp
    SettingsBackend.cpp
)

qt_add_qml_module(lightwm-settings
    URI LightWM.Settings
    VERSION 1.0
    QML_FILES
        qml/SettingsWindow.qml
)

target_link_libraries(lightwm-settings PRIVATE
    lightwm_core
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
)

install(TARGETS lightwm-settings DESTINATION bin)
