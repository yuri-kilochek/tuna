cmake_minimum_required(VERSION 3.11)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(GNUInstallDirs)
include(WriteFileIfChanged)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS VERSION)
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PROJECT_VERSION)
project(tuna VERSION ${PROJECT_VERSION})

add_library(tuna)

set_target_properties(tuna PROPERTIES VERSION ${PROJECT_VERSION})
if("${BUILD_SHARED_LIBS}")
    set_target_properties(tuna PROPERTIES C_VISIBILITY_PRESET hidden)
    target_compile_definitions(tuna PRIVATE TUNA_EXPORT INTERFACE TUNA_IMPORT)
endif()

set(HEADER_TEMPLATE "${CMAKE_CURRENT_SOURCE_DIR}/include/tuna.h.in")
set(HEADER "${CMAKE_CURRENT_BINARY_DIR}/include/tuna.h")
set(HEADER_GENERATOR "${CMAKE_CURRENT_BINARY_DIR}/tuna.h.generator.cmake")
write_file_if_changed("${HEADER_GENERATOR}"
    "set(MAJOR_VERSION ${PROJECT_VERSION_MAJOR})\n"
    "set(MINOR_VERSION ${PROJECT_VERSION_MINOR})\n"
    "set(PATCH_VERSION ${PROJECT_VERSION_PATCH})\n"
    "configure_file(\"${HEADER_TEMPLATE}\" \"${HEADER}\")\n"
)
add_custom_command(
    MAIN_DEPENDENCY "${HEADER_TEMPLATE}"
    DEPENDS "${HEADER_GENERATOR}"
    VERBATIM COMMAND "${CMAKE_COMMAND}" -P "${HEADER_GENERATOR}"
    OUTPUT "${HEADER}"
)
target_sources(tuna PRIVATE "${HEADER}")
target_include_directories(tuna
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
    PRIVATE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)

target_sources(tuna PRIVATE
    src/version.c
    src/error.c
    $<$<PLATFORM_ID:Linux>:src/linux.c>
    $<$<PLATFORM_ID:Windows>:src/windows.c>
)

if("${CMAKE_SYSTEM_NAME}" STREQUAL Linux)
    find_package(libnl REQUIRED)
    target_link_libraries(tuna PRIVATE libnl::libnl)
endif()
if("${CMAKE_SYSTEM_NAME}" STREQUAL Windows)
    add_subdirectory(deps/tap-windows)
    embed_tap_windows(tuna)
    target_link_libraries(tuna PRIVATE shlwapi setupapi newdev)
endif()

target_include_directories(tuna PUBLIC
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/include/tuna.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
install(TARGETS tuna EXPORT tuna
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
install(EXPORT tuna
    DESTINATION "${CMAKE_INSTALL_DATADIR}/tuna"
    NAMESPACE tuna::
    FILE tuna-config.cmake
)
export(EXPORT tuna
    NAMESPACE tuna::
    FILE tuna-config.cmake
)
