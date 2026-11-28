set(RIVER_PROTOCOLS_DIR "/usr/share/river-protocols/stable")
set(WAYLAND_PROTOCOLS_DIR "/usr/share/wayland-protocols")

set(LIGHTWM_PROTOCOL_XMLS
    "${RIVER_PROTOCOLS_DIR}/river-window-management-v1.xml"
    "${RIVER_PROTOCOLS_DIR}/river-layer-shell-v1.xml"
    "${RIVER_PROTOCOLS_DIR}/river-xkb-bindings-v1.xml"
    "${RIVER_PROTOCOLS_DIR}/river-xkb-config-v1.xml"
    "${RIVER_PROTOCOLS_DIR}/river-input-management-v1.xml"
    "${RIVER_PROTOCOLS_DIR}/river-libinput-config-v1.xml"
    "${WAYLAND_PROTOCOLS_DIR}/staging/ext-session-lock/ext-session-lock-v1.xml"
    "${WAYLAND_PROTOCOLS_DIR}/staging/fractional-scale/fractional-scale-v1.xml"
    "${WAYLAND_PROTOCOLS_DIR}/stable/viewporter/viewporter.xml"
    "${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/wlr-output-management-unstable-v1.xml"
)

set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${GENERATED_DIR}")

set(PROTOCOL_SOURCES "")
set(PROTOCOL_HEADERS "")

foreach(XML_PATH IN LISTS LIGHTWM_PROTOCOL_XMLS)
    get_filename_component(PROTO_NAME "${XML_PATH}" NAME_WE)
    set(HEADER_FILE "${GENERATED_DIR}/${PROTO_NAME}-client-protocol.h")
    set(CODE_FILE "${GENERATED_DIR}/${PROTO_NAME}-protocol.c")

    add_custom_command(
        OUTPUT "${HEADER_FILE}"
        COMMAND "${WAYLAND_SCANNER}" client-header "${XML_PATH}" "${HEADER_FILE}"
        DEPENDS "${XML_PATH}"
        COMMENT "Generating Wayland client header for ${PROTO_NAME}"
    )

    add_custom_command(
        OUTPUT "${CODE_FILE}"
        COMMAND "${WAYLAND_SCANNER}" private-code "${XML_PATH}" "${CODE_FILE}"
        DEPENDS "${XML_PATH}"
        COMMENT "Generating Wayland protocol code for ${PROTO_NAME}"
    )

    list(APPEND PROTOCOL_HEADERS "${HEADER_FILE}")
    list(APPEND PROTOCOL_SOURCES "${CODE_FILE}")
endforeach()

add_library(lightwm_protocols STATIC
    ${PROTOCOL_SOURCES}
    ${PROTOCOL_HEADERS}
)

target_include_directories(lightwm_protocols PUBLIC
    "${GENERATED_DIR}"
)

target_link_libraries(lightwm_protocols PUBLIC
    PkgConfig::WAYLAND_CLIENT
)
