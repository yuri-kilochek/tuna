#ifndef TUNA_PRIV_INCLUDE_GUARD_CARRIER_STATUS
#define TUNA_PRIV_INCLUDE_GUARD_CARRIER_STATUS

#include <tuna/api.h>
#include <tuna/error.h>
#include <tuna/ref.h>

#include <stdint.h>

///////////////////////////////////////////////////////////////////////////////

typedef uint_least8_t tuna_carrier_status;
#define TUNA_DISCONNECTED UINT8_C(0)
#define TUNA_CONNECTED    UINT8_C(1)

TUNA_PRIV_API
tuna_error
tuna_set_carrier_status(tuna_ref *ref, tuna_carrier_status status);

TUNA_PRIV_API
tuna_error
tuna_get_carrier_status(tuna_ref const *ref, tuna_carrier_status *status_out);

///////////////////////////////////////////////////////////////////////////////

#endif
