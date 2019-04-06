include_guard(GLOBAL)

include(WriteFileIfChanged)

function(embed_file TARGET VAR FILE)
    get_filename_component(FILE "${FILE}" ABSOLUTE)
    get_filename_component(NAME "${FILE}" NAME)

    string(MD5 TAG "${FILE}")
    set(SOURCE "${CMAKE_BINARY_DIR}/EmbeddedFiles/${TAG}.c")
    set(SOURCE_GENERATOR "${SOURCE}.generator.cmake")
    write_file_if_changed("${SOURCE_GENERATOR}"
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
    add_custom_command(
        COMMENT "Sourcifying ${NAME}"
        MAIN_DEPENDENCY "${FILE}"
        DEPENDS "${SOURCE_GENERATOR}"
        VERBATIM COMMAND "${CMAKE_COMMAND}" -P "${SOURCE_GENERATOR}"
        OUTPUT "${SOURCE}"
    )
    target_sources("${TARGET}" PRIVATE "${SOURCE}")
endfunction()
