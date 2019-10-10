#ifndef TUNA_PRIV_INCLUDE_GUARD_OWNERSHIP
#define TUNA_PRIV_INCLUDE_GUARD_OWNERSHIP

#include <tuna/error.h>
#include <tuna/ref.h>
#include <tuna/api.h>

#include <stdint.h>

///////////////////////////////////////////////////////////////////////////////

typedef uint_least8_t tuna_ownership;
#define TUNA_EXCLUSIVE UINT8_C(0)
#define TUNA_SHARED    UINT8_C(1)

TUNA_PRIV_API
tuna_error
tuna_get_ownership(tuna_ref const *ref, tuna_ownership *ownership_out);

///////////////////////////////////////////////////////////////////////////////

#endif
