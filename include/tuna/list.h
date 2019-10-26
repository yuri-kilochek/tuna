#ifndef TUNA_PRIV_INCLUDE_GUARD_LIST
#define TUNA_PRIV_INCLUDE_GUARD_LIST

#include <tuna/api.h>
#include <tuna/error.h>
#include <tuna/ref.h>

#include <stddef.h>

///////////////////////////////////////////////////////////////////////////////

typedef struct tuna_list tuna_list;

TUNA_PRIV_API
tuna_error
tuna_get_list(tuna_list **list_out);

TUNA_PRIV_API
void
tuna_free_list(tuna_list *list);

TUNA_PRIV_API
size_t
tuna_get_count(tuna_list const *list);

TUNA_PRIV_API
tuna_error
tuna_bind_at(tuna_ref *ref, tuna_list const *list, size_t position);

///////////////////////////////////////////////////////////////////////////////

#endif
