include_guard(GLOBAL)

include(WriteFileIfChanged)

function(_tuna_embed_file TARGET VAR FILE)
    get_filename_component(FILE "${FILE}" ABSOLUTE)
    get_filename_component(NAME "${FILE}" NAME)

    string(MD5 TAG "${VAR}")
    set(SOURCE "${CMAKE_BINARY_DIR}/EmbeddedFiles/${TAG}.c")
    set(SOURCE_GENERATOR "${SOURCE}.generator.cmake")
    _tuna_write_file_if_changed("${SOURCE_GENERATOR}"
        "file(READ \"${FILE}\" DATA HEX)\n"
        "string(LENGTH \${DATA} DOUBLE_SIZE)\n"
        "math(EXPR SIZE \"\${DOUBLE_SIZE} / 2\")\n"
        "string(REGEX REPLACE \"(..)\" \"0x\\\\1,\" DATA \${DATA})\n"

        "file(WRITE \"${SOURCE}\"\n"
        "    \"#include <tuna/priv/windows/embedded_driver.h>\\n\"\n"
        
        "    \"tuna_embedded_file const ${VAR} = {\\n\"\n"
        "    \"    .name = L\\\"${NAME}\\\",\\n\"\n"
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

function(_tuna_embed_driver TARGET VAR INF CAT SYS)
    string(MD5 TAG "${VAR}")

    set(INF_VAR "tuna_embedded_driver_${TAG}_inf")
    set(CAT_VAR "tuna_embedded_driver_${TAG}_cat")
    set(SYS_VAR "tuna_embedded_driver_${TAG}_sys")

    _tuna_embed_file("${TARGET}" "${INF_VAR}" "${INF}")
    _tuna_embed_file("${TARGET}" "${CAT_VAR}" "${CAT}")
    _tuna_embed_file("${TARGET}" "${SYS_VAR}" "${SYS}")

    set(SOURCE "${CMAKE_BINARY_DIR}/EmbeddedDrivers/${TAG}.c")
    _tuna_write_file_if_changed("${SOURCE}"
        "#include <tuna/priv/windows/embedded_driver.h>\n"

        "extern tuna_embedded_file const ${INF_VAR};\n"
        "extern tuna_embedded_file const ${CAT_VAR};\n"
        "extern tuna_embedded_file const ${SYS_VAR};\n"

        "tuna_embedded_driver const ${VAR} = {\n"
        "    .inf_file = &${INF_VAR},\n"
        "    .cat_file = &${CAT_VAR},\n"
        "    .sys_file = &${SYS_VAR},\n"
        "};\n"
    )
    target_sources("${TARGET}" PRIVATE "${SOURCE}")
endfunction()
