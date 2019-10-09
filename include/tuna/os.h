#ifndef TUNA_PRIV_INCLUDE_GUARD_OS
#define TUNA_PRIV_INCLUDE_GUARD_OS

///////////////////////////////////////////////////////////////////////////////

#if defined(_WIN32) \
 || defined(_WIN64) \
 || defined(__WIN32__) \
 || defined(__TOS_WIN__) \
 || defined(__WINDOWS__)
    #define TUNA_PRIV_OS_WINDOWS 1
#endif

///////////////////////////////////////////////////////////////////////////////

#endif
