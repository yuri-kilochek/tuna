#ifndef TUNA_IMPL_INCL_IMPL_API_H
#define TUNA_IMPL_INCL_IMPL_API_H

#include <tuna/impl/os.h>

#include <tuna/impl/prolog.inc>
///////////////////////////////////////////////////////////////////////////////

#if TUNA_USE_SHARED && TUNA_BUILD_SHARED
    #error TUNA_USE_SHARED and TUNA_BUILD_SHARED must not be defined \
        simultaneously
#endif

#if TUNA_IMPL_OS_WINDOWS
    #if TUNA_USE_SHARED
        #define TUNA_IMPL_API __declspec(dllimport)
    #elif TUNA_BUILD_SHARED
        #define TUNA_IMPL_API __declspec(dllexport)
    #else
        #define TUNA_IMPL_API 
    #endif
#else
    #if TUNA_BUILD_SHARED
        #define TUNA_IMPL_API __attribute__((visibility("default")))
    #else
        #define TUNA_IMPL_API
    #endif
#endif

///////////////////////////////////////////////////////////////////////////////
#include <tuna/impl/epilog.inc>

#endif

