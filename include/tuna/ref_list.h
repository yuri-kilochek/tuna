#ifndef TUNA_PRIV_INCLUDE_GUARD_REF_LIST
#define TUNA_PRIV_INCLUDE_GUARD_REF_LIST

#include <tuna/error.h>
#include <tuna/ref.h>
#include <tuna/api.h>

#include <stddef.h>

///////////////////////////////////////////////////////////////////////////////

typedef struct tuna_ref_list tuna_ref_list;

TUNA_PRIV_API
tuna_error
tuna_get_ref_list(tuna_ref_list **list_out);

TUNA_PRIV_API
void
tuna_free_ref_list(tuna_ref_list *list);

TUNA_PRIV_API
size_t
tuna_get_ref_count(tuna_ref_list const *list);

TUNA_PRIV_API
tuna_error
tuna_bind_like_at(tuna_ref *ref,
                  tuna_ref_list const *example_list, size_t example_position);

///////////////////////////////////////////////////////////////////////////////

#endif
