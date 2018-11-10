#ifndef INTERFACEPTOR_DETAIL_INCLUDED_VIRTUAL_INTERFACE
#define INTERFACEPTOR_DETAIL_INCLUDED_VIRTUAL_INTERFACE

#include <boost/predef/os.h>

#if BOOST_OS_LINUX
    #include <interfaceptor/detail/virtual_interface_linux.hpp>
#else
    #error "not implemented"
#endif


#endif
