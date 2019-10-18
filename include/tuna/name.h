#ifndef TUNA_PRIV_INCLUDE_GUARD_NAME
#define TUNA_PRIV_INCLUDE_GUARD_NAME

#include <tuna/api.h>
#include <tuna/error.h>
#include <tuna/ref.h>

///////////////////////////////////////////////////////////////////////////////

TUNA_PRIV_API
tuna_error
tuna_set_name(tuna_ref *ref, char const *name);

TUNA_PRIV_API
tuna_error
tuna_get_name(tuna_ref const *ref, char **name_out);

TUNA_PRIV_API
void
tuna_free_name(char *name);

TUNA_PRIV_API
tuna_error
tuna_bind_by_name(tuna_ref *ref, char const *name);

///////////////////////////////////////////////////////////////////////////////

#endif
