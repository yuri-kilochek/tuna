#ifndef TUNA_PRIV_INCLUDE_GUARD_CONNECTION
#define TUNA_PRIV_INCLUDE_GUARD_CONNECTION

#include <tuna/error.h>
#include <tuna/ref.h>
#include <tuna/api.h>

#include <stdint.h>

///////////////////////////////////////////////////////////////////////////////

typedef uint_fast8_t tuna_connection;
#define TUNA_DISCONNECTED UINT8_C(0)
#define TUNA_CONNECTED    UINT8_C(1)

TUNA_PRIV_API
tuna_error
tuna_set_connection(tuna_ref *ref, tuna_connection connection);

TUNA_PRIV_API
tuna_error
tuna_get_connection(tuna_ref const *ref, tuna_connection *connection_out);

///////////////////////////////////////////////////////////////////////////////

#endif
