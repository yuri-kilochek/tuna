#ifndef TUNA_IMPL_INCL_HANDLE_H
#define TUNA_IMPL_INCL_HANDLE_H

#include <tuna/impl/os.h>

#include <tuna/impl/prolog.inc>
///////////////////////////////////////////////////////////////////////////////

#if TUNA_IMPL_OS_WINDOWS
    typedef void *tuna_handle; // aka HANDLE
#else
    typedef int tuna_handle;
#endif

///////////////////////////////////////////////////////////////////////////////
#include <tuna/impl/epilog.inc>

#endif

