set(NAME "tap-windows-9.22.1")
set(URL "https://build.openvpn.net/downloads/releases/${NAME}.zip")
set(SHA256 "d15fb9a502375ebf46de769e611bb477c41cffeba27789e9406183a81553249c")
set(ARCHIVE "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.zip")
if(NOT EXISTS "${ARCHIVE}")
    message("-- Downloading ${URL}:")
    file(DOWNLOAD "${URL}" "${ARCHIVE}"
        SHOW_PROGRESS
        EXPECTED_HASH "SHA256=${SHA256}"
        STATUS STATUS
    )
    list(GET STATUS 0 STATUS_CODE)
    if(STATUS_CODE)
        list(GET STATUS 1 STATUS_MESSAGE)
        message(FATAL_ERROR "Download failed: ${STATUS_MESSAGE}")
    endif()
endif()

set(CONTENT_DIR "${CMAKE_CURRENT_BINARY_DIR}/${NAME}")
if(NOT EXISTS "${CONTENT_DIR}")
    execute_process(
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        COMMAND ${CMAKE_COMMAND} -E tar xfz "${ARCHIVE}"
        RESULT_VARIABLE RESULT
    )
    if(RESULT)
        file(REMOVE_RECURSE "${CONTENT_DIR}")
        message(FATAL_ERROR "Decompression of ${NAME} failed: ${RESULT}")
    endif()
endif()

set(INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/include")
set(SOURCE "${CMAKE_CURRENT_BINARY_DIR}/files.c")

if(NOT _tap-windows_FILES_SOURCIFIED)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(FILES_DIR "amd64")
    else()
        set(FILES_DIR "i386")
    endif()
    set(FILES_DIR "${CONTENT_DIR}/${FILES_DIR}")

    file(MAKE_DIRECTORY "${INCLUDE_DIR}")
    set(HEADER "${INCLUDE_DIR}/tap-windows-files.h")
    string(RANDOM LENGTH 64 TOKEN)
    file(WRITE "${HEADER}"
        "#ifndef TAP_WINDOWS_INCLUDE_GUARD_${TOKEN}\n"
        "#define TAP_WINDOWS_INCLUDE_GUARD_${TOKEN}\n"
    )

    file(REMOVE "${SOURCE}")

    foreach(NAME OemVista.inf tap0901.cat tap0901.sys)
        file(READ "${FILES_DIR}/${NAME}" DATA HEX)
        string(LENGTH "${DATA}" SIZE)
        math(EXPR SIZE "${SIZE} / 2")
        string(REGEX REPLACE "(..)" "0x\\1," DATA "${DATA}")

        string(REPLACE "." ";" EXT "${NAME}")
        list(REVERSE EXT)
        list(GET EXT 0 EXT)

        file(APPEND "${HEADER}"
            "extern char const *tap_windows_${EXT}_name;\n"
            "extern unsigned char const tap_windows_${EXT}[${SIZE}];\n"
        )
        file(APPEND "${SOURCE}"
            "char const *tap_windows_${EXT}_name = \"${NAME}\";\n"
            "unsigned char const tap_windows_${EXT}[] = {${DATA}};\n"
        )
    endforeach()

    file(APPEND "${HEADER}"
        "#endif\n"
    )

    set(_tap-windows_FILES_SOURCIFIED 1 CACHE INTERNAL "")
endif()

add_library(tap-windows INTERFACE IMPORTED GLOBAL)
target_sources(tap-windows INTERFACE "${SOURCE}")
target_include_directories(tap-windows INTERFACE
    "${CONTENT_DIR}/include"
    "${INCLUDE_DIR}"    
)
