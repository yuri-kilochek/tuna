cmake_minimum_required(VERSION 3.8)

include(GNUInstallDirs)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PROJECT_VERSION)
project(tuna VERSION ${PROJECT_VERSION})

add_library(tuna
    "${CMAKE_CURRENT_BINARY_DIR}/include/tuna.h"
    src/generic.c
    src/linux.c
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

add_custom_command(
    OUTPUT include/tuna.h
    MAIN_DEPENDENCY include/tuna.h.in
    COMMAND "${CMAKE_COMMAND}"
        -D INPUT="${CMAKE_CURRENT_SOURCE_DIR}/include/tuna.h.in" 
        -D OUTPUT="${CMAKE_CURRENT_BINARY_DIR}/include/tuna.h" 
        -D MAJOR_VERSION="${PROJECT_VERSION_MAJOR}" 
        -D MINOR_VERSION="${PROJECT_VERSION_MINOR}" 
        -D PATCH_VERSION="${PROJECT_VERSION_PATCH}" 
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/configure_file.cmake"
)

target_include_directories(tuna PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

set_target_properties(tuna PROPERTIES
    VERSION ${PROJECT_VERSION}
)

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
