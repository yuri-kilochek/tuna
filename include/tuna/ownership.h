#ifndef TUNA_IMPL_INCL_OWNERSHIP_H
#define TUNA_IMPL_INCL_OWNERSHIP_H

#include <stdint.h>

#include <tuna/impl/prolog.inc>
///////////////////////////////////////////////////////////////////////////////

typedef uint_least8_t tuna_ownership;

#define TUNA_OWNERSHIP_EXCLUSIVE UINT8_C(1)
#define TUNA_OWNERSHIP_SHARED    UINT8_C(2)

///////////////////////////////////////////////////////////////////////////////
#include <tuna/impl/epilog.inc>

#endif

