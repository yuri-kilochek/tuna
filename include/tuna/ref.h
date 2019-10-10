#ifndef TUNA_PRIV_INCLUDE_GUARD_REF
#define TUNA_PRIV_INCLUDE_GUARD_REF

#include <tuna/error.h>
#include <tuna/api.h>

#include <stdint.h>

///////////////////////////////////////////////////////////////////////////////

typedef struct tuna_ref tuna_ref;

TUNA_PRIV_API
tuna_error
tuna_get_ref(tuna_ref **ref_out);

TUNA_PRIV_API
void
tuna_free_ref(tuna_ref *ref);

TUNA_PRIV_API
tuna_error
tuna_bind_like(tuna_ref_ref *ref, tuna_ref_ref const *example_ref);

TUNA_PRIV_API
uint_fast8_t
tuna_is_bound(tuna_ref const *ref);

TUNA_PRIV_API
void
tuna_unbind(tuna_ref *ref);

///////////////////////////////////////////////////////////////////////////////

#endif
