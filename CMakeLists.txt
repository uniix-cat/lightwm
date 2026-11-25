cmake_minimum_required(VERSION 3.24)
project(lightwm VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(PkgConfig REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    DBus
    Network
    WaylandClient
    WaylandClientPrivate
)
find_package(LayerShellQt REQUIRED)
if(COMMAND qt_policy)
    qt_policy(SET QTP0001 NEW)
    qt_policy(SET QTP0004 NEW)
endif()

pkg_check_modules(WAYLAND_CLIENT REQUIRED IMPORTED_TARGET wayland-client)
pkg_check_modules(XKBCOMMON REQUIRED IMPORTED_TARGET xkbcommon)
pkg_check_modules(LIBPULSE REQUIRED IMPORTED_TARGET libpulse)
pkg_check_modules(PAM REQUIRED IMPORTED_TARGET pam)
pkg_check_modules(LIBINPUT REQUIRED IMPORTED_TARGET libinput)
pkg_check_modules(LIBUDEV REQUIRED IMPORTED_TARGET libudev)

find_program(WAYLAND_SCANNER wayland-scanner REQUIRED)

add_subdirectory(protocols)
add_subdirectory(src/core)
add_subdirectory(src/wm)
add_subdirectory(src/shell)
add_subdirectory(src/settings)

install(PROGRAMS
    packaging/lightwm-session
    packaging/lightwm-init
    DESTINATION bin
)

install(FILES
    packaging/lightwm.desktop
    DESTINATION share/wayland-sessions
)

install(FILES
    packaging/lightwm-settings.desktop
    DESTINATION share/applications
)

install(FILES
    packaging/71-lightwm-touchpad.rules
    DESTINATION lib/udev/rules.d
)
