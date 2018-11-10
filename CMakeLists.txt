cmake_minimum_required(VERSION 3.8)

file(STRINGS "VERSION" PROJECT_VERSION)
project(interfaceptor VERSION "${PROJECT_VERSION}")

add_library(interfaceptor INTERFACE)

find_package(Boost REQUIRED)
target_link_libraries(interfaceptor INTERFACE
    Boost::boost
    Boost::system
)

target_include_directories(interfaceptor INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:$include>
)

install(TARGETS interfaceptor EXPORT interfaceptor)
install(EXPORT interfaceptor DESTINATION cmake FILE interfaceptor-config.cmake)
export(EXPORT interfaceptor FILE interfaceptor-config.cmake)
