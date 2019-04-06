target_include_directories(tuna PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
)

set(HEADER_IN "${CMAKE_CURRENT_SOURCE_DIR}/tuna.h.in")
set(HEADER "${CMAKE_CURRENT_BINARY_DIR}/tuna.h")
set(CONFIGURE "${CMAKE_CURRENT_BINARY_DIR}/configure.cmake")

file(WRITE "${CONFIGURE}.new"
    "set(MAJOR_VERSION ${PROJECT_VERSION_MAJOR})\n"
    "set(MINOR_VERSION ${PROJECT_VERSION_MINOR})\n"
    "set(PATCH_VERSION ${PROJECT_VERSION_PATCH})\n"
    "configure_file(\"${HEADER_IN}\" \"${HEADER}\")\n"
)
execute_process(
    ERROR_QUIET
    COMMAND "${CMAKE_COMMAND}" -E compare_files "${CONFIGURE}.new"
                                                "${CONFIGURE}"
    RESULT_VARIABLE CHANGED
)
if("${CHANGED}")
    file(RENAME "${CONFIGURE}.new" "${CONFIGURE}")
endif()

add_custom_command(
    MAIN_DEPENDENCY "${HEADER_IN}"
    DEPENDS "${CONFIGURE}"
    VERBATIM
    COMMAND "${CMAKE_COMMAND}" -P "${CONFIGURE}"
    OUTPUT "${HEADER}"
)
file(TOUCH "${HEADER}")
target_sources(tuna PRIVATE "${HEADER}")
target_include_directories(tuna PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
)

# CMake can't track file dependencies in subdirectories :<
add_custom_target(tuna_header
    COMMENT ""
    DEPENDS "${HEADER}"
)
add_dependencies(tuna tuna_header)
