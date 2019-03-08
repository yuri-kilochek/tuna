function(embed_resource TARGET BUNDLE_HEADER RESOURCE NAME)
    get_filename_component(RESOURCE "${RESOURCE}" ABSOLUTE)
    string(MD5 TAG "${RESOURCE}")
    set(NAMESPACE "${CMAKE_CURRENT_BINARY_DIR}/${TAG}")

    set(SOURCE "${NAMESPACE}.c")
    set(HEADER "${NAMESPACE}.h")

    set(SOURCIFY "${NAMESPACE}.cmake")
    file(WRITE "${SOURCIFY}"
        "file(READ \"${RESOURCE}\" RESOURCE HEX)\n"
        "string(LENGTH \"\${RESOURCE}\" SIZE)\n"
        "math(EXPR SIZE \"\${SIZE} / 2\")\n"
        "string(REGEX REPLACE \"(..)\" \"0x\\\\1,\""
        " RESOURCE \"\${RESOURCE}\""
        ")\n"
        "file(WRITE \"${HEADER}\"\n"
        "    \"extern unsigned char const data_${TAG}[\${SIZE}];\\n\"\n"
        ")\n"
        "file(WRITE \"${SOURCE}\"\n"
        "    \"unsigned char const data_${TAG}[]={\${RESOURCE}};\\n\"\n"
        ")\n"
    )
    add_custom_command(
        MAIN_DEPENDENCY "${RESOURCE}"
        VERBATIM COMMAND "${CMAKE_COMMAND}" -P "${SOURCIFY}"
        OUTPUT "${SOURCE}" "${HEADER}"
    )
    target_sources(${TARGET} PRIVATE "${HEADER}" "${SOURCE}")



endfunction()
