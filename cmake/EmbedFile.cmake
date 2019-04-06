function(embed_file TARGET VAR FILE)
    get_filename_component(FILE "${FILE}" ABSOLUTE)
    get_filename_component(NAME "${FILE}" NAME)

    set(DIR "${CMAKE_BINARY_DIR}/EmbeddedFiles")
    file(MAKE_DIRECTORY "${DIR}")

    string(MD5 TAG "${FILE}")
    set(SOURCE "${DIR}/${TAG}.c")
    set(SOURCIFY "${DIR}/${TAG}.cmake")

    file(WRITE "${SOURCIFY}.new"
        "file(READ \"${FILE}\" DATA HEX)\n"
        "string(LENGTH \${DATA} DOUBLE_SIZE)\n"
        "math(EXPR SIZE \"\${DOUBLE_SIZE} / 2\")\n"
        "string(REGEX REPLACE \"(..)\" \"0x\\\\1,\" DATA \${DATA})\n"

        "file(WRITE \"${SOURCE}\"\n"
        "    \"#include <tuna/priv/embedded_file.h>\\n\"\n"
        
        "    \"tuna_embedded_file const ${VAR} = {\\n\"\n"
        "    \"    .name = \\\"${NAME}\\\",\\n\"\n"
        "    \"    .size = \${SIZE},\\n\"\n"
        "    \"    .data = (unsigned char const[]){\${DATA}},\\n\"\n"
        "    \"};\\n\"\n"
        ")\n"
    )
    execute_process(
        ERROR_QUIET
        COMMAND "${CMAKE_COMMAND}" -E compare_files "${SOURCIFY}.new"
                                                    "${SOURCIFY}"
        RESULT_VARIABLE CHANGED
    )
    if("${CHANGED}")
        file(RENAME "${SOURCIFY}.new" "${SOURCIFY}")
    endif()

    add_custom_command(
        COMMENT "Sourcifying ${NAME}"
        MAIN_DEPENDENCY "${FILE}"
        DEPENDS "${SOURCIFY}"
        VERBATIM
        COMMAND "${CMAKE_COMMAND}" -P "${SOURCIFY}"
        OUTPUT "${SOURCE}"
    )
    file(TOUCH "${SOURCE}")
    target_sources("${TARGET}" PRIVATE "${SOURCE}")

    # CMake can't track file dependencies in subdirectories :<
    set(SOURCE_TARGET "embedded_file_${TAG}")
    add_custom_target("${SOURCE_TARGET}"
        COMMENT ""
        DEPENDS "${SOURCE}"
    )
    add_dependencies("${TARGET}" "${SOURCE_TARGET}")
endfunction()
