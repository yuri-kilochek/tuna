include_guard(GLOBAL)

include(WriteFileIfChanged)

function(_tuna_embed_file TARGET VAR FILE)
    string(MD5 TAG "${VAR}")
    set(SOURCE "${CMAKE_BINARY_DIR}/EmbeddedFiles/${TAG}.c")
    set(SOURCE_GENERATOR "${SOURCE}.generator.cmake")
    _tuna_write_file_if_changed("${SOURCE_GENERATOR}"
        "get_filename_component(NAME \"\${FILE}\" NAME)\n"

        "file(READ \"\${FILE}\" DATA HEX)\n"
        "string(LENGTH \${DATA} DOUBLE_SIZE)\n"
        "math(EXPR SIZE \"\${DOUBLE_SIZE} / 2\")\n"
        "string(REGEX REPLACE \"(..)\" \"0x\\\\1,\" DATA \${DATA})\n"

        "file(WRITE \"${SOURCE}\"\n"
        "    \"#include <tuna/priv/windows/embedded_driver.h>\\n\"\n"
        
        "    \"tuna_embedded_file const ${VAR} = {\\n\"\n"
        "    \"    .name = L\\\"\${NAME}\\\",\\n\"\n"
        "    \"    .size = \${SIZE},\\n\"\n"
        "    \"    .data = (unsigned char const[]){\${DATA}},\\n\"\n"
        "    \"};\\n\"\n"
        ")\n"
    )
    add_custom_command(
        COMMENT "Sourcifying ${FILE}"
        DEPENDS "${FILE}" "${SOURCE_GENERATOR}"
        VERBATIM COMMAND "${CMAKE_COMMAND}"
            -D "FILE=${FILE}"
            -P "${SOURCE_GENERATOR}"
        OUTPUT "${SOURCE}"
    )
    target_sources("${TARGET}" PRIVATE "${SOURCE}")
endfunction()
