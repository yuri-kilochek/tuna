#ifndef TUNA_PRIV_INCLUDE_GUARD_API
#define TUNA_PRIV_INCLUDE_GUARD_API

#include <tuna/os.h>

///////////////////////////////////////////////////////////////////////////////

#if __cplusplus
    #define TUNA_PRIV_API_LANGUAGE_LINKAGE extern "C"
#else
    #define TUNA_PRIV_API_LANGUAGE_LINKAGE
#endif

#if defined(TUNA_IMPORT) && defined(TUNA_EXPORT)
    #error "TUNA_IMPORT and TUNA_EXPORT must not be defined simultaneously"
#endif
#if TUNA_PRIV_OS_WINDOWS
    #if defined(TUNA_IMPORT)
        #define TUNA_PRIV_API_VISIBILITY __declspec(dllimport)
    #elif defined(TUNA_EXPORT)
        #define TUNA_PRIV_API_VISIBILITY __declspec(dllexport)
    #else
        #define TUNA_PRIV_API_VISIBILITY 
    #endif
#else
    #if defined(TUNA_EXPORT)
        #define TUNA_PRIV_API_VISIBILITY __attribute__((visibility("default")))
    #else
        #define TUNA_PRIV_API_VISIBILITY 
    #endif
#endif

#define TUNA_PRIV_API TUNA_PRIV_API_LANGUAGE_LINKAGE TUNA_PRIV_API_VISIBILITY

///////////////////////////////////////////////////////////////////////////////

#endif
