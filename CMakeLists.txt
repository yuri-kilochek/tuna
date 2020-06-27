cmake_minimum_required(VERSION 3.17)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS VERSION)
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PROJECT_VERSION)
project(tuna VERSION ${PROJECT_VERSION} LANGUAGES C)

include(GNUInstallDirs)


add_library(tuna)

set_target_properties(tuna PROPERTIES VERSION ${PROJECT_VERSION})
if(BUILD_SHARED_LIBS)
    set_target_properties(tuna PROPERTIES C_VISIBILITY_PRESET hidden)
    target_compile_definitions(tuna
        PRIVATE TUNA_BUILD_DYNAMIC=1
        INTERFACE TUNA_USE_DYNAMIC=1
    )
endif()

set(VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(VERSION_PATCH ${PROJECT_VERSION_PATCH})
configure_file(include/tuna/version.h.in 
               include/tuna/version.h)

target_include_directories(tuna PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)

target_sources(tuna PRIVATE
    src/version.c
    src/error.c
    src/string.c
    src/address_list.c
    src/ref_list.c
)
if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    target_sources(tuna PRIVATE
        src/linux/error.c
        src/linux/netlink.c
        src/linux/ref.c
        src/linux/ref_list.c
    )
endif()
if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    target_sources(tuna PRIVATE
        src/windows.c
    )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    find_package(libnl REQUIRED)
    target_link_libraries(tuna PRIVATE libnl::libnl)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    add_subdirectory(deps/tap-windows)
    _tuna_embed_tap_windows(tuna)

    add_subdirectory(src/windows/janitor)
    _tuna_embed_janitor(tuna)

    target_link_libraries(tuna PRIVATE pathcch setupapi newdev)
endif()


target_include_directories(tuna PUBLIC
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
              "${CMAKE_CURRENT_BINARY_DIR}/include/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    PATTERN "*.in" EXCLUDE
    PATTERN "tuna/priv"   EXCLUDE
    PATTERN "tuna/priv/*" EXCLUDE
)
install(TARGETS tuna EXPORT tuna)
install(EXPORT tuna
    DESTINATION "${CMAKE_INSTALL_DATADIR}/tuna"
    NAMESPACE tuna::
    FILE tuna-config.cmake
)
export(EXPORT tuna
    NAMESPACE tuna::
    FILE tuna-config.cmake
)

