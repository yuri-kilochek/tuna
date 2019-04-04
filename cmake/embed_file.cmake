set(_EMBEDDED_FILE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include"
    CACHE INTERNAL ""    
)

function(embed_file TARGET VAR PATH)
    get_filename_component(PATH "${PATH}" ABSOLUTE)
    get_filename_component(NAME "${PATH}" NAME)

    string(MD5 TAG "${PATH}")
    set(SOURCE "${CMAKE_BINARY_DIR}/embedded_file/${TAG}.c")
    set(SOURCIFY "${SOURCE}.generator.cmake")

    target_include_directories("${TARGET}" PRIVATE
        "${_EMBED_FILE_INCLUDE_DIR}"
    )

    file(WRITE "${SOURCIFY}.new"
        "file(READ \"${PATH}\" DATA HEX)\n"
        "string(LENGTH \${DATA} DOUBLE_SIZE)\n"
        "math(EXPR SIZE \"\${DOUBLE_SIZE} / 2\")\n"
        "string(REGEX REPLACE \"(..)\" \"0x\\\\1,\" DATA \${DATA})\n"

        "file(WRITE \"${SOURCE}\"\n"
        "    \"#include <embedded_file.h>\\n\"\n"
        
        "    \"embedded_file const ${VAR} = {\\n\"\n"
        "    \"    .name = \\\"${NAME}\\\",\\n\"\n"
        "    \"    .size = \${SIZE},\\n\"\n"
        "    \"    .data = (unsigned char const[]){\${DATA}},\\n\"\n"
        "    \"};\\n\"\n"
        ")\n"
    )
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E compare_files "${SOURCIFY}.new" "${SOURCIFY}"
        ERROR_QUIET
        RESULT_VARIABLE CHANGED
    )
    if("${CHANGED}")
        file(RENAME "${SOURCIFY}.new" "${SOURCIFY}")
    endif()

    add_custom_command(
        COMMENT "Sourcifying ${NAME}"
        MAIN_DEPENDENCY "${PATH}"
        DEPENDS "${SOURCIFY}"
        VERBATIM COMMAND "${CMAKE_COMMAND}" -P "${SOURCIFY}"
        OUTPUT "${SOURCE}"
    )
    target_sources("${TARGET}" PRIVATE "${SOURCE}")
endfunction()
