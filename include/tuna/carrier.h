#ifndef TUNA_PRIV_INCLUDE_GUARD_CARRIER
#define TUNA_PRIV_INCLUDE_GUARD_CARRIER

#include <tuna/api.h>
#include <tuna/error.h>
#include <tuna/ref.h>

#include <stdint.h>

///////////////////////////////////////////////////////////////////////////////

typedef uint_least8_t tuna_carrier;
#define TUNA_DISCONNECTED UINT8_C(0)
#define TUNA_CONNECTED    UINT8_C(1)

TUNA_PRIV_API
tuna_error
tuna_set_carrier(tuna_ref *ref, tuna_carrier carrier);

TUNA_PRIV_API
tuna_error
tuna_get_carrier(tuna_ref const *ref, tuna_carrier *carrier_out);

///////////////////////////////////////////////////////////////////////////////

#endif
