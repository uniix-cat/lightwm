add_executable(lightwm-wm
    main.cpp
    WindowManager.hpp
    WindowManager.cpp
    LayoutEngine.hpp
    LayoutEngine.cpp
    IpcServer.hpp
    IpcServer.cpp
    TouchpadGestures.hpp
    TouchpadGestures.cpp
    CornerDecorations.hpp
    CornerDecorations.cpp
    OutputScaleManager.hpp
    OutputScaleManager.cpp
)

target_link_libraries(lightwm-wm PRIVATE
    lightwm_core
    lightwm_protocols
    PkgConfig::WAYLAND_CLIENT
    PkgConfig::XKBCOMMON
    PkgConfig::LIBINPUT
    PkgConfig::LIBUDEV
)

install(TARGETS lightwm-wm DESTINATION bin)
