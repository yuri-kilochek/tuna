target_sources(tuna PRIVATE
    version.c
    error.c
)
if("${CMAKE_SYSTEM_NAME}" STREQUAL Linux)
    target_sources(tuna PRIVATE
        linux.c
    )
endif()
if("${CMAKE_SYSTEM_NAME}" STREQUAL Windows)
    target_sources(tuna PRIVATE
        windows.c
    )
    add_subdirectory(windows)
endif()


