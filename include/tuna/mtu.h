#ifndef TUNA_PRIV_INCLUDE_GUARD_MTU
#define TUNA_PRIV_INCLUDE_GUARD_MTU

#include <tuna/error.h>
#include <tuna/ref.h>
#include <tuna/api.h>

#include <stddef.h>

///////////////////////////////////////////////////////////////////////////////

TUNA_PRIV_API
tuna_error
tuna_set_mtu(tuna_ref *ref, size_t mtu);

TUNA_PRIV_API
tuna_error
tuna_get_mtu(tuna_ref const *ref, size_t *mtu_out);

///////////////////////////////////////////////////////////////////////////////

#endif
