function(write_file_if_changed FILE)
    string(MD5 TMP_FILE "${FILE};${ARGN}")
    set(TMP_FILE "${CMAKE_CURRENT_BINARY_DIR}/${TMP_FILE}")

    file(WRITE "${TMP_FILE}" ${ARGN})
    execute_process(
        ERROR_QUIET
        COMMAND "${CMAKE_COMMAND}" -E compare_files "${TMP_FILE}" "${FILE}"
        RESULT_VARIABLE CHANGED
    )
    if("${CHANGED}")
        file(RENAME "${TMP_FILE}" "${FILE}")
    else()
        file(REMOVE "${TMP_FILE}")
    endif()
endfunction()
