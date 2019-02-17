cmake_minimum_required(VERSION 3.8)

include(GNUInstallDirs)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PROJECT_VERSION)
project(tuna VERSION ${PROJECT_VERSION})

add_library(tuna
    src/version.c
    src/error.c
    src/linux.c
)

string(CONFIGURE [=[
    set(MAJOR_VERSION ${PROJECT_VERSION_MAJOR})
    set(MINOR_VERSION ${PROJECT_VERSION_MINOR})
    set(PATCH_VERSION ${PROJECT_VERSION_PATCH})
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/include/tuna.h.in"
                   "${CMAKE_CURRENT_BINARY_DIR}/include/tuna.h")
]=] CONFIGURE_SCRIPT)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/configure.cmake" "${CONFIGURE_SCRIPT}")
add_custom_command(
    MAIN_DEPENDENCY include/tuna.h.in
    VERBATIM COMMAND "${CMAKE_COMMAND}" -P configure.cmake
    OUTPUT include/tuna.h
)
target_sources(tuna PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/include/tuna.h"
)

target_include_directories(tuna PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

if(BUILD_SHARED_LIBS)
    target_compile_definitions(tuna 
        PRIVATE TUNA_EXPORT
        INTERFACE TUNA_IMPORT
    )
    set_target_properties(tuna PROPERTIES
        C_VISIBILITY_PRESET hidden
    )
endif()

set_target_properties(tuna PROPERTIES
    VERSION ${PROJECT_VERSION}
)

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(libnl REQUIRED IMPORTED_TARGET
        libnl-3.0
        libnl-route-3.0
    )
    target_link_libraries(tuna PRIVATE
        PkgConfig::libnl
    )
endif()

install(TARGETS tuna
    EXPORT tuna
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/include/tuna.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
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
