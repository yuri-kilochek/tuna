# No attempt is made to handle anything other than the invocation in 
# /CMakeLists.txt

find_path(_LIBNL_INCLUDE_DIR NAMES netlink/netlink.h PATH_SUFFIXES libnl3)
if(NOT _LIBNL_INCLUDE_DIR)
    message(FATAL_ERROR "Failed to find libnl include directory.")
endif()

find_library(_LIBNL nl-3)
if(NOT _LIBNL)
    message(FATAL_ERROR "Failed to find libnl.")
endif()

find_library(_LIBNL_ROUTE nl-route-3)
if(NOT _LIBNL_ROUTE)
    message(FATAL_ERROR "Failed to find libnl-route.")
endif()

add_library(libnl::libnl INTERFACE IMPORTED)
target_include_directories(libnl::libnl INTERFACE "${_LIBNL_INCLUDE_DIR}")
target_link_libraries(libnl::libnl INTERFACE "${_LIBNL}" "${_LIBNL_ROUTE}")
set(libnl_FOUND YES)
