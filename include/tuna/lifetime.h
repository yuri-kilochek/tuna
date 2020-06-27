#ifndef TUNA_IMPL_INCL_LIFETIME_H
#define TUNA_IMPL_INCL_LIFETIME_H

#include <stdint.h>

#include <tuna/impl/prolog.inc>
///////////////////////////////////////////////////////////////////////////////

typedef uint_least8_t tuna_lifetime;

#define TUNA_LIFETIME_TRANSIENT  UINT8_C(1)
#define TUNA_LIFETIME_PERSISTENT UINT8_C(2)

///////////////////////////////////////////////////////////////////////////////
#include <tuna/impl/epilog.inc>

#endif

