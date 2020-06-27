#ifndef TUNA_IMPL_INCL_IMPL_OS_H
#define TUNA_IMPL_INCL_IMPL_OS_H

#include <tuna/impl/prolog.inc>
///////////////////////////////////////////////////////////////////////////////

#if defined(_WIN32) \
 || defined(_WIN64) \
 || defined(__WIN32__) \
 || defined(__TOS_WIN__) \
 || defined(__WINDOWS__)
    #define TUNA_IMPL_OS_WINDOWS 1
#endif

///////////////////////////////////////////////////////////////////////////////
#include <tuna/impl/epilog.inc>

#endif

