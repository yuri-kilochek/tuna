cmake_minimum_required(VERSION 3.13)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(GNUInstallDirs)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PROJECT_VERSION)
project(tuna VERSION ${PROJECT_VERSION})

add_library(tuna)

set_target_properties(tuna PROPERTIES VERSION ${PROJECT_VERSION})
if(BUILD_SHARED_LIBS)
    set_target_properties(tuna PROPERTIES C_VISIBILITY_PRESET hidden)
    target_compile_definitions(tuna PRIVATE TUNA_EXPORT INTERFACE TUNA_IMPORT)
endif()

add_subdirectory(include)
add_subdirectory(src)

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    find_package(libnl REQUIRED)
    target_link_libraries(tuna PRIVATE libnl::libnl)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    add_subdirectory(deps/tap-windows)
endif()

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
