include_guard(GLOBAL)

function(write_file_if_changed FILE)
    string(MD5 TMP_FILE "${FILE};${ARGN}")
    set(TMP_FILE "${CMAKE_CURRENT_BINARY_DIR}/${TMP_FILE}")

    set(CONTENT "")
    math(EXPR LAST_ARG_INDEX "${ARGC} - 1")
    if("${LAST_ARG_INDEX}" GREATER 0)
        foreach(I RANGE 1 "${LAST_ARG_INDEX}")
            set(CONTENT "${CONTENT}${ARGV${I}}")
        endforeach()
    endif()
    file(WRITE "${TMP_FILE}" "${CONTENT}")

    execute_process(
        ERROR_QUIET
        COMMAND "${CMAKE_COMMAND}" -E compare_files "${TMP_FILE}" "${FILE}"
        RESULT_VARIABLE CHANGED
    )
    if("${CHANGED}")
        get_filename_component(DIR "${FILE}" DIRECTORY)
        file(MAKE_DIRECTORY "${DIR}")
        file(RENAME "${TMP_FILE}" "${FILE}")
    else()
        file(REMOVE "${TMP_FILE}")
    endif()
endfunction()
