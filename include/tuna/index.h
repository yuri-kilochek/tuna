#ifndef TUNA_PRIV_INCLUDE_GUARD_INDEX
#define TUNA_PRIV_INCLUDE_GUARD_INDEX

#include <tuna/error.h>
#include <tuna/ref.h>
#include <tuna/api.h>

///////////////////////////////////////////////////////////////////////////////

TUNA_PRIV_API
tuna_error
tuna_get_index(tuna_ref const *ref, unsigned *index_out);

TUNA_PRIV_API
tuna_error
tuna_bind_by_index(tuna_ref *ref, unsigned index);

///////////////////////////////////////////////////////////////////////////////

#endif
