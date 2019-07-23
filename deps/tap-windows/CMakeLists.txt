include(Windows/EmbedDriver)

set(TUNA_TAP_WINDOWS_URL
    https://build.openvpn.net/downloads/releases/tap-windows-9.22.1.zip
    CACHE STRING "Where to get archive with compiled TAP-Windows driver from."
)
set(URL "${TUNA_TAP_WINDOWS_URL}")

set(TUNA_TAP_WINDOWS_HASH
    SHA256=D15FB9A502375EBF46DE769E611BB477C41CFFEBA27789E9406183A81553249C
    CACHE STRING "Hash of the archive at TUNA_TAP_WINDOWS_URL."
)
set(HASH "${TUNA_TAP_WINDOWS_HASH}")

file(READ "${CMAKE_CURRENT_LIST_FILE}" THIS_CODE)

set(CACHE_TAG "")
string(SHA256 CACHE_TAG "${CACHE_TAG}${URL}")
string(SHA256 CACHE_TAG "${CACHE_TAG}${HASH}")
string(SHA256 CACHE_TAG "${CACHE_TAG}${THIS_CODE}")
if(NOT ("${_TUNA_TAP_WINDOWS_CACHE_TAG}" STREQUAL "${CACHE_TAG}"))
    if("${HASH}")
        set(EXPECTED_HASH_ARG EXPECTED_HASH "${HASH}")
    else()
        set(EXPECTED_HASH_ARG)
    endif()

    set(ARCHIVE "${CMAKE_CURRENT_BINARY_DIR}/archive")
    message(STATUS "Downloading TAP-Windows driver...")
    file(DOWNLOAD "${URL}" "${ARCHIVE}"
        SHOW_PROGRESS
        ${EXPECTED_HASH_ARG}
        STATUS STATUS
    )
    list(GET STATUS 0 CODE)
    if("${CODE}")
        list(GET STATUS 1 MESSAGE)
        message(FATAL_ERROR "Download failed: ${MESSAGE}")
    endif()

    set(CONTENT_DIR "${CMAKE_CURRENT_BINARY_DIR}/content")
    file(MAKE_DIRECTORY "${CONTENT_DIR}")
    execute_process(
        WORKING_DIRECTORY "${CONTENT_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E tar xf "${ARCHIVE}"
        RESULT_VARIABLE CODE
    )
    if("${CODE}")
        message(FATAL_ERROR "Failed to extract TAP-windows archive.")
    endif()

    file(GLOB CONTENT_DIR "${CONTENT_DIR}/tap-windows-*.*.*/")

    set(INCLUDE_DIR "${CONTENT_DIR}/include")

    if("${CMAKE_SIZEOF_VOID_P}" EQUAL 4)
        set(ARCH i386)
    else()
        set(ARCH amd64)
    endif()
    set(ARCH_DIR "${CONTENT_DIR}/${ARCH}")

    file(GLOB INF "${ARCH_DIR}/*.inf")
    file(GLOB CAT "${ARCH_DIR}/*.cat")
    file(GLOB SYS "${ARCH_DIR}/*.sys")

    set(_TUNA_TAP_WINDOWS_INCLUDE_DIR "${INCLUDE_DIR}"  CACHE INTERNAL "" FORCE)
    set(_TUNA_TAP_WINDOWS_INF "${INF}" CACHE INTERNAL "" FORCE)
    set(_TUNA_TAP_WINDOWS_CAT "${CAT}" CACHE INTERNAL "" FORCE)
    set(_TUNA_TAP_WINDOWS_SYS "${SYS}" CACHE INTERNAL "" FORCE)

    set(HARDWARE_ID)
    foreach(ENCODING IN ITEMS "UTF-16LE" "UTF-8")
        file(STRINGS "${INF}" INF_CONTENT ENCODING "${ENCODING}")
        list(JOIN INF_CONTENT "\n" INF_CONTENT)
        if("${INF_CONTENT}" MATCHES "\n\\[([^\n]+)\.driver]\n")
            set(HARDWARE_ID "${CMAKE_MATCH_1}")
            break()
        endif()
    endforeach()
    if("${HARDWARE_ID}" STREQUAL "")
        message(FATAL_ERROR "Can't extract hardware id from TAP-Windows'"
                            " .inf file")
    endif()
    set(_TUNA_TAP_WINDOWS_HARDWARE_ID "${HARDWARE_ID}" CACHE INTERNAL "" FORCE)

    set(_TUNA_TAP_WINDOWS_CACHE_TAG "${CACHE_TAG}" CACHE INTERNAL "")
endif()

function(_tuna_embed_tap_windows TARGET)
    target_include_directories("${TARGET}" PRIVATE
        "${_TUNA_TAP_WINDOWS_INCLUDE_DIR}"
    )
    _tuna_embed_driver("${TARGET}" tuna_priv_embedded_tap_windows
        "${_TUNA_TAP_WINDOWS_HARDWARE_ID}"
        "${_TUNA_TAP_WINDOWS_INF}"
        "${_TUNA_TAP_WINDOWS_CAT}"
        "${_TUNA_TAP_WINDOWS_SYS}"
    )
endfunction()
