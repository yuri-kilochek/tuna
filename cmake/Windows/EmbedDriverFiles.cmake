include_guard(GLOBAL)

include(EmbedFile)
include(WriteFileIfChanged)

function(embed_driver_files TARGET VAR INF CAT SYS)
    get_filename_component(INF "${INF}" ABSOLUTE)
    get_filename_component(CAT "${CAT}" ABSOLUTE)
    get_filename_component(SYS "${SYS}" ABSOLUTE)

    string(MD5 TAG "${VAR}")

    set(INF_VAR "tuna_embedded_driver_${TAG}_inf")
    set(CAT_VAR "tuna_embedded_driver_${TAG}_cat")
    set(SYS_VAR "tuna_embedded_driver_${TAG}_sys")

    embed_file("${TARGET}" "${INF_VAR}" "${INF}")
    embed_file("${TARGET}" "${CAT_VAR}" "${CAT}")
    embed_file("${TARGET}" "${SYS_VAR}" "${SYS}")

    set(SOURCE "${CMAKE_BINARY_DIR}/EmbeddedDriverFiles/${TAG}.c")
    write_file_if_changed("${SOURCE}"
        "#include <tuna/priv/windows/embedded_driver_files.h>\n"

        "extern tuna_embedded_file const ${INF_VAR};\n"
        "extern tuna_embedded_file const ${CAT_VAR};\n"
        "extern tuna_embedded_file const ${SYS_VAR};\n"

        "tuna_embedded_driver_files const ${VAR} = {\n"
        "    &${INF_VAR},\n"
        "    &${CAT_VAR},\n"
        "    &${SYS_VAR},\n"
        "};\n"
    )
    target_sources("${TARGET}" PRIVATE "${SOURCE}")
endfunction()
