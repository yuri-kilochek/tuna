include(EmbedFile)

function(embed_driver TARGET VAR INF CAT SYS)
    get_filename_component(INF "${INF}" ABSOLUTE)
    get_filename_component(CAT "${CAT}" ABSOLUTE)
    get_filename_component(SYS "${SYS}" ABSOLUTE)

    string(MD5 TAG "${INF}\n${CAT}\n${SYS}")

    set(INF_VAR "tuna_embedded_driver_${TAG}_inf")
    set(CAT_VAR "tuna_embedded_driver_${TAG}_cat")
    set(SYS_VAR "tuna_embedded_driver_${TAG}_sys")

    embed_file("${TARGET}" "${INF_VAR}" "${INF}")
    embed_file("${TARGET}" "${CAT_VAR}" "${CAT}")
    embed_file("${TARGET}" "${SYS_VAR}" "${SYS}")

    set(DIR "${CMAKE_BINARY_DIR}/EmbeddedDrivers")
    file(MAKE_DIRECTORY "${DIR}")

    set(SOURCE "${DIR}/${TAG}.c")
    file(WRITE "${SOURCE}.new"
        "#include <tuna/priv/windows/embedded_driver.h>\n"

        "extern tuna_embedded_file const ${INF_VAR};\n"
        "extern tuna_embedded_file const ${CAT_VAR};\n"
        "extern tuna_embedded_file const ${SYS_VAR};\n"

        "tuna_embedded_driver const ${VAR} = {\n"
        "    &${INF_VAR},\n"
        "    &${CAT_VAR},\n"
        "    &${SYS_VAR},\n"
        "};\n"
    )
    execute_process(
        ERROR_QUIET
        COMMAND "${CMAKE_COMMAND}" -E compare_files "${SOURCE}.new" "${SOURCE}"
        RESULT_VARIABLE CHANGED
    )
    if("${CHANGED}")
        file(RENAME "${SOURCE}.new" "${SOURCE}")
    endif()
    target_sources("${TARGET}" PRIVATE "${SOURCE}")
endfunction()
