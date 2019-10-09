#ifndef TUNA_PRIV_INCLUDE_GUARD_IO_HANDLE
#define TUNA_PRIV_INCLUDE_GUARD_IO_HANDLE

#include <tuna/ref.h>
#include <tuna/os.h>
#include <tuna/api.h>

///////////////////////////////////////////////////////////////////////////////

#if TUNA_PRIV_OS_WINDOWS
    typedef void *tuna_io_handle;
#else
    typedef int tuna_io_handle;
#endif

TUNA_PRIV_API
tuna_io_handle
tuna_get_io_handle(tuna_ref *ref);

///////////////////////////////////////////////////////////////////////////////

#endif
