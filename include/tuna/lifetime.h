#ifndef TUNA_PRIV_INCLUDE_GUARD_LIFETIME
#define TUNA_PRIV_INCLUDE_GUARD_LIFETIME

#include <tuna/error.h>
#include <tuna/ref.h>
#include <tuna/api.h>

#include <stdint.h>

///////////////////////////////////////////////////////////////////////////////

typedef uint_fast8_t tuna_lifetime;
#define TUNA_TRANSIENT  UINT8_C(0)
#define TUNA_PERSISTENT UINT8_C(1)

TUNA_PRIV_API
tuna_error
tuna_set_lifetime(tuna_ref *ref, tuna_lifetime lifetime);

TUNA_PRIV_API
tuna_error
tuna_get_lifetime(tuna_ref const *ref, tuna_lifetime *lifetime_out);

///////////////////////////////////////////////////////////////////////////////

#endif
