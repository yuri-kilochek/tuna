cmake_minimum_required(VERSION 3.8)

include(GNUInstallDirs)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PROJECT_VERSION)
project(tuna VERSION "${PROJECT_VERSION}")

add_library(tuna INTERFACE)

find_package(Boost 1.66 REQUIRED COMPONENTS
    system
)
target_link_libraries(tuna INTERFACE
    Boost::boost
    Boost::system
)

target_include_directories(tuna INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

install(TARGETS tuna
    EXPORT tuna
)
install(DIRECTORY "include/tuna"
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
