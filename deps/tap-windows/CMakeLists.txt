set(NAME tap-windows-9.22.1)
set(URL https://build.openvpn.net/downloads/releases/${NAME}.zip)
set(SHA256 d15fb9a502375ebf46de769e611bb477c41cffeba27789e9406183a81553249c)

set(ARCHIVE "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.zip")

set(DOWNLOAD "${CMAKE_CURRENT_BINARY_DIR}/download.cmake")
file(WRITE "${DOWNLOAD}"
    "file(DOWNLOAD ${URL} \"${ARCHIVE}\"\n"
    "    TLS_VERIFY YES\n"
    "    SHOW_PROGRESS\n"
    "    EXPECTED_HASH SHA256=${SHA256}\n"
    "    STATUS STATUS\n"
    ")\n"
    "list(GET STATUS 0 STATUS_CODE)\n"
    "if(STATUS_CODE)\n"
    "    list(GET STATUS 1 STATUS)\n"
    "    message(FATAL_ERROR \"Download failed: \${STATUS}\")\n"
    "endif()\n"
)
add_custom_command(
    COMMENT "Downloading ${URL}"
    VERBATIM COMMAND "${CMAKE_COMMAND}" -P "${DOWNLOAD}"
    OUTPUT "${ARCHIVE}"
)

set(CONTENT_DIR "${CMAKE_CURRENT_BINARY_DIR}/${NAME}")

set(INCLUDE_DIR "${CONTENT_DIR}/include")
set(HEADER "${INCLUDE_DIR}/tap-windows.h")

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH amd64)
else()
    set(ARCH i386)
endif()
set(INF "${CONTENT_DIR}/${ARCH}/OemVista.inf")
set(CAT "${CONTENT_DIR}/${ARCH}/tap0901.cat")
set(SYS "${CONTENT_DIR}/${ARCH}/tap0901.sys")

add_custom_command(
    COMMENT "Unpacking ${NAME}.zip"
    MAIN_DEPENDENCY "${ARCHIVE}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    VERBATIM COMMAND "${CMAKE_COMMAND}" -E tar xfz "${ARCHIVE}"
    VERBATIM COMMAND "${CMAKE_COMMAND}" -E touch "${HEADER}"
    VERBATIM COMMAND "${CMAKE_COMMAND}" -E touch "${INF}"
    VERBATIM COMMAND "${CMAKE_COMMAND}" -E touch "${CAT}"
    VERBATIM COMMAND "${CMAKE_COMMAND}" -E touch "${SYS}"
    OUTPUT "${HEADER}" "${INF}" "${CAT}" "${SYS}"
)

add_library(tap-windows STATIC "${HEADER}")

target_include_directories(tap-windows INTERFACE
    "$<BUILD_INTERFACE:${INCLUDE_DIR}>"
)

foreach(FILE "${INF}" "${CAT}" "${SYS}")
    file(WRITE "${FILE}.cmake"
        "set(FILE \"${FILE}\")\n"
        "file(READ \"\${FILE}\" DATA HEX)\n"
        "string(LENGTH \${DATA} DOUBLE_SIZE)\n"
        "math(EXPR SIZE \"\${DOUBLE_SIZE} / 2\")\n"
        "string(REGEX REPLACE \"(..)\" \"0x\\\\1,\" DATA \${DATA})\n"

        "get_filename_component(NAME \"\${FILE}\" NAME)\n"
        "get_filename_component(EXT \"\${NAME}\" EXT)\n"
        "string(SUBSTRING \${EXT} 1 -1 EXT)\n"

        "file(WRITE \"\${FILE}.c\"\n"
        "    \"#include <stddef.h>\\n\"\n"
        "    \"char const *tap_windows_\${EXT}_name = \\\"\${NAME}\\\";\\n\"\n"
        "    \"size_t const tap_windows_\${EXT}_size = \${SIZE};\\n\"\n"
        "    \"unsigned char const *const tap_windows_\${EXT}_data ="
                 " (unsigned char const[]){\${DATA}};\\n\"\n"
        ")\n"
    )
    add_custom_command(
        MAIN_DEPENDENCY "${FILE}"
        VERBATIM COMMAND "${CMAKE_COMMAND}" -P "${FILE}.cmake"
        OUTPUT "${FILE}.c"
    )

    target_sources(tap-windows PRIVATE "${FILE}.c")
endforeach()
