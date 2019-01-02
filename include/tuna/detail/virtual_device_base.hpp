#ifndef TUNA_DETAIL_INCLUDED_DETAIL_VIRTUAL_DEVICE_BASE
#define TUNA_DETAIL_INCLUDED_DETAIL_VIRTUAL_DEVICE_BASE

#include <boost/predef/os.h>

#if BOOST_OS_LINUX
    #include <tuna/detail/impl/virtual_device_base/linux.hpp>
#else
    #error "not implemented"
#endif

#endif
