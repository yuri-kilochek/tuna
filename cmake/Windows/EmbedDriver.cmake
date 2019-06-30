include_guard(GLOBAL)

include(Windows/EmbedFile)
include(WriteFileIfChanged)

function(_tuna_embed_driver TARGET VAR HARDWARE_ID INF_PATH CAT_PATH SYS_PATH)
    string(MD5 TAG "${VAR}")

    set(INF_VAR "tuna_priv_embedded_driver_${TAG}_inf")
    set(CAT_VAR "tuna_priv_embedded_driver_${TAG}_cat")
    set(SYS_VAR "tuna_priv_embedded_driver_${TAG}_sys")

    _tuna_embed_file("${TARGET}" "${INF_VAR}" "${INF_PATH}")
    _tuna_embed_file("${TARGET}" "${CAT_VAR}" "${CAT_PATH}")
    _tuna_embed_file("${TARGET}" "${SYS_VAR}" "${SYS_PATH}")

    set(SOURCE "${CMAKE_BINARY_DIR}/EmbeddedDrivers/${TAG}.c")
    _tuna_write_file_if_changed("${SOURCE}"
        "#include <tuna/priv/windows/embedded_driver.h>\n"

        "extern tuna_embedded_file const ${INF_VAR};\n"
        "extern tuna_embedded_file const ${CAT_VAR};\n"
        "extern tuna_embedded_file const ${SYS_VAR};\n"

        "tuna_embedded_driver const ${VAR} = {\n"
        "    .hardware_id = L\"${HARDWARE_ID}\",\n"
        "    .inf_file = &${INF_VAR},\n"
        "    .cat_file = &${CAT_VAR},\n"
        "    .sys_file = &${SYS_VAR},\n"
        "};\n"
    )
    target_sources("${TARGET}" PRIVATE "${SOURCE}")
endfunction()
