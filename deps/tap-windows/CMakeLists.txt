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
set(EMBEDDED_FILES_HEADER "${INCLUDE_DIR}/tap-windows-embedded-files.h")

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

add_library(tap-windows INTERFACE IMPORTED GLOBAL)

file(MAKE_DIRECTORY "${INCLUDE_DIR}")
target_include_directories(tap-windows INTERFACE "${INCLUDE_DIR}")
file(TOUCH "${HEADER}")
file(WRITE "${EMBEDDED_FILES_HEADER}"
    "#ifndef TAP_WINDOWS_EMBEDDED_FILES_INCLUDED\n"
    "#define TAP_WINDOWS_EMBEDDED_FILES_INCLUDED\n"

    "#include <stddef.h>\n"

    "typedef struct {\n"
    "    char const *pszName;\n"
    "    size_t cbSize;\n"
    "    unsigned char const *pData;\n"
    "} TAP_WINDOWS_EMBEDDED_FILE;\n"
)

foreach(FILE "${INF}" "${CAT}" "${SYS}")
    get_filename_component(EXT "${FILE}" EXT)
    string(SUBSTRING "${EXT}" 1 1 E)
    string(SUBSTRING "${EXT}" 2 -1 XT)
    string(TOUPPER "${E}" E)
    set(EXT "${E}${XT}")

    set(DECL "TAP_WINDOWS_EMBEDDED_FILE const g_TapWindowsEmbedded${EXT}File")

    file(APPEND "${EMBEDDED_FILES_HEADER}"
        "extern ${DECL};\n"
    )
    file(WRITE "${FILE}.cmake"
        "set(FILE \"${FILE}\")\n"

        "get_filename_component(NAME \"\${FILE}\" NAME)\n"

        "file(READ \"\${FILE}\" DATA HEX)\n"
        "string(LENGTH \${DATA} DOUBLE_SIZE)\n"
        "math(EXPR SIZE \"\${DOUBLE_SIZE} / 2\")\n"
        "string(REGEX REPLACE \"(..)\" \"0x\\\\1,\" DATA \${DATA})\n"

        "file(WRITE \"\${FILE}.c\"\n"
        "    \"#include <${EMBEDDED_FILES_HEADER}>\\n\"\n"
        
        "    \"${DECL} = {\\n\"\n"
        "    \"    .pszName = \\\"\${NAME}\\\",\\n\"\n"
        "    \"    .cbSize = \${SIZE},\\n\"\n"
        "    \"    .pData = (unsigned char const[]){\${DATA}},\\n\"\n"
        "    \"};\\n\"\n"
        ")\n"
    )
    add_custom_command(
        MAIN_DEPENDENCY "${FILE}"
        VERBATIM COMMAND "${CMAKE_COMMAND}" -P "${FILE}.cmake"
        OUTPUT "${FILE}.c"
    )

    file(TOUCH "${FILE}.c")
    target_sources(tap-windows INTERFACE "${FILE}.c")
endforeach()

file(APPEND "${EMBEDDED_FILES_HEADER}"
    "#endif\n"
)
target_sources(tap-windows INTERFACE "${HEADER}" "${EMBEDDED_FILES_HEADER}")

# This silliness is only required because CMake (as of 3.14) is apparently
# unable to track dependencies through INTERFACE_SOURCES of an INTERFACE
# library. And we also can't just make tap-windows a regular library because
# then CMake will force us to export it, even though it is fully internal.
add_custom_target(tap-windows-gen
    COMMENT ""
    DEPENDS "${HEADER}" "${INF}.c" "${CAT}.c" "${SYS}.c"
)
add_dependencies(tap-windows tap-windows-gen)
