#ifndef TUNA_PRIV_INCLUDE_GUARD
#define TUNA_PRIV_INCLUDE_GUARD

#include <tuna/version.h>
#include <tuna/api.h>
#include <tuna/error.h>
#include <tuna/ref.h>
#include <tuna/ref_list.h>
#include <tuna/ownership.h>
#include <tuna/lifetime.h>
#include <tuna/index.h>
#include <tuna/name.h>
#include <tuna/mtu.h>
#include <tuna/address.h>
#include <tuna/address_list.h>
#include <tuna/carrier.h>
#include <tuna/io_handle.h>

#include <stdint.h>

///////////////////////////////////////////////////////////////////////////////

TUNA_PRIV_API
tuna_error
tuna_open(tuna_ref *ref, tuna_ownership ownership);

TUNA_PRIV_API
uint_fast8_t
tuna_is_open(tuna_ref const *ref);

TUNA_PRIV_API
void
tuna_close(tuna_ref *ref);

///////////////////////////////////////////////////////////////////////////////

#endif
