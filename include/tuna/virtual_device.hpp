#ifndef TUNA_DETAIL_INCLUDED_VIRTUAL_DEVICE
#define TUNA_DETAIL_INCLUDED_VIRTUAL_DEVICE

#include <boost/predef/os.h>

#if BOOST_OS_WINDOWS
    #include <tuna/impl/virtual_device/windows.hpp>
#elif BOOST_OS_LINUX
    #include <tuna/impl/virtual_device/linux.hpp>
#else
    #error "not implemented"
#endif

#endif
