add_library(lightwm_core STATIC
    Config.hpp
    Config.cpp
    Theme.hpp
    Theme.cpp
    IpcProtocol.hpp
)

target_include_directories(lightwm_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(lightwm_core PUBLIC
    Qt6::Core
    Qt6::Gui
)
