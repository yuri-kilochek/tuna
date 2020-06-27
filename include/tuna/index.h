#ifndef TUNA_IMPL_INCL_INDEX_H
#define TUNA_IMPL_INCL_INDEX_H

#include <tuna/impl/os.h>

#include <tuna/impl/prolog.inc>
///////////////////////////////////////////////////////////////////////////////

#if TUNA_IMPL_OS_WINDOWS
    typedef unsigned long tuna_index; // aka NET_IFINDEX
#else
    typedef unsigned int tuna_index;
#endif

///////////////////////////////////////////////////////////////////////////////
#include <tuna/impl/epilog.inc>

#endif

