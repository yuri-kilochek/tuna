cmake_minimum_required(VERSION 3.8)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(GNUInstallDirs)
include(EmbedResource)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PROJECT_VERSION)
project(tuna VERSION ${PROJECT_VERSION})

add_library(tuna
    src/version.c
    src/error.c
    $<$<PLATFORM_ID:Linux>:src/linux.c>
)

set_target_properties(tuna PROPERTIES
    VERSION ${PROJECT_VERSION}
)

set(CONFIGURE "${CMAKE_CURRENT_BINARY_DIR}/configure.cmake")
file(WRITE "${CONFIGURE}"
    "set(MAJOR_VERSION ${PROJECT_VERSION_MAJOR})\n"
    "set(MINOR_VERSION ${PROJECT_VERSION_MINOR})\n"
    "set(PATCH_VERSION ${PROJECT_VERSION_PATCH})\n"
    "configure_file(\"${CMAKE_CURRENT_SOURCE_DIR}/include/tuna.h.in\"\n"
    "               \"${CMAKE_CURRENT_BINARY_DIR}/include/tuna.h\")\n"
)
add_custom_command(
    MAIN_DEPENDENCY include/tuna.h.in
    VERBATIM COMMAND "${CMAKE_COMMAND}" -P "${CONFIGURE}"
    OUTPUT include/tuna.h
)
target_include_directories(tuna PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
)
target_sources(tuna PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/include/tuna.h"
)

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    find_package(libnl REQUIRED)
    target_link_libraries(tuna PRIVATE
        libnl::libnl
    )
endif()

embed_resource(tuna tuna/driver.h CMakeLists.txt tuna_driver_sys)
embed_resource(tuna tuna/driver.h conanfile.py tuna_driver_inf)

if(BUILD_SHARED_LIBS)
    target_compile_definitions(tuna 
        PRIVATE TUNA_EXPORT
        INTERFACE TUNA_IMPORT
    )
    set_target_properties(tuna PROPERTIES
        C_VISIBILITY_PRESET hidden
    )
endif()

target_include_directories(tuna PUBLIC
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/include/tuna.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
install(TARGETS tuna
    EXPORT tuna
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
