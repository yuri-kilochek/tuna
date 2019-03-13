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

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args("libnl include directory"
    REQUIRED_VARS _nl_INCLUDE_DIR
)
find_package_handle_standard_args("libnl binary"
    REQUIRED_VARS _nl_LIBRARY
)
find_package_handle_standard_args("libnl-route binary"
    REQUIRED_VARS _nl-route_LIBRARY
)

if(_nl_INCLUDE_DIR AND _nl_LIBRARY AND _nl-route_LIBRARY)
    set(libnl_FOUND YES)

    add_library(libnl::libnl INTERFACE IMPORTED)
    target_include_directories(libnl::libnl INTERFACE "${_nl_INCLUDE_DIR}")
    target_link_libraries(libnl::libnl INTERFACE
        "${_nl_LIBRARY}"
        "${_nl-route_LIBRARY}"
    )
endif()
