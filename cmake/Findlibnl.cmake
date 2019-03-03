include(FindPackageHandleStandardArgs)

find_path(_nl_INCLUDE_DIR
    NAMES netlink/netlink.h
    PATH_SUFFIXES libnl3
)
find_library(_nl_LIBRARY
    NAMES nl-3
)
find_library(_nl-route_LIBRARY
    NAMES nl-route-3
)

find_package_handle_standard_args(libnl DEFAULT_MSG
    _nl_LIBRARY
    _nl-route_LIBRARY
    _nl_INCLUDE_DIR
)

if(libnl_FOUND)
    add_library(libnl::libnl INTERFACE IMPORTED)
    target_include_directories(libnl::libnl INTERFACE
        "${_nl_INCLUDE_DIR}"
    )
    target_link_libraries(libnl::libnl INTERFACE
        "${_nl_LIBRARY}"
        "${_nl-route_LIBRARY}"
    )
endif()
