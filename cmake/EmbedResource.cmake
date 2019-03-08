function(embed_resource TARGET BUNDLE_HEADER RESOURCE NAME)
    get_filename_component(RESOURCE "${RESOURCE}" ABSOLUTE)
    string(MD5 TAG "${RESOURCE}")
    set(NAMESPACE "${CMAKE_BINARY_DIR}/embedded_resources/${TAG}")

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
        "set(NAME \"embedded_resource_${TAG}\")\n"
        "file(WRITE \"${HEADER}\"\n"
        "    \"#ifndef INCLUDE_${TAG}\\n\"\n"
        "    \"#define INCLUDE_${TAG}\\n\"\n"
        "    \"extern unsigned char const \${NAME}[\${SIZE}];\\n\"\n"
        "    \"#endif\\n\"\n"
        ")\n"
        "file(WRITE \"${SOURCE}\"\n"
        "    \"unsigned char const \${NAME}[] = {\${RESOURCE}};\\n\"\n"
        ")\n"
    )
    add_custom_command(
        MAIN_DEPENDENCY "${RESOURCE}" DEPENDS "${SOURCIFY}"
        VERBATIM COMMAND "${CMAKE_COMMAND}" -P "${SOURCIFY}"
        OUTPUT "${SOURCE}" "${HEADER}"
    )

    get_target_property(SOURCES "${TARGET}" SOURCES)

    list(FIND SOURCES "${SOURCE}" INDEX)
    if(INDEX EQUAL -1)
        target_sources("${TARGET}" PRIVATE "${HEADER}" "${SOURCE}")
    endif()

    set(INCLUDE "${CMAKE_CURRENT_BINARY_DIR}/embedded_resources/${TARGET}")
    set(BUNDLE_HEADER "${INCLUDE}/${BUNDLE_HEADER}")
    list(FIND SOURCES "${BUNDLE_HEADER}" INDEX)
    if(INDEX EQUAL -1)
        target_sources("${TARGET}" PRIVATE "${BUNDLE_HEADER}")
        target_include_directories("${TARGET}" PRIVATE "${INCLUDE}")
        get_filename_component(DIRECTORY "${BUNDLE_HEADER}" DIRECTORY)
        file(MAKE_DIRECTORY "${DIRECTORY}")
        file(REMOVE "${BUNDLE_HEADER}")
    endif()
    file(APPEND "${BUNDLE_HEADER}"
        "#include <${HEADER}>\n"    
        "#define ${NAME} embedded_resource_${TAG}\n"    
    )
endfunction()
