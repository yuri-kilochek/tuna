find_package(PkgConfig REQUIRED)

if(libnl_FIND_REQURIED)
    set(_required REQUIRED)
else()
    set(_required)
endif()

if(libnl_FIND_QUIETLY)
    set(_quiet QUIET)
else()
    set(_quiet)
endif()

pkg_check_modules(_libnl ${_required} ${_quiet} IMPORTED_TARGET
    libnl-3.0
    libnl-route-3.0
)

add_library(libnl::libnl INTERFACE IMPORTED)
target_link_libraries(libnl::libnl INTERFACE
    PkgConfig::_libnl
)
