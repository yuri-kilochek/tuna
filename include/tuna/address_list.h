#ifndef TUNA_PRIV_INCLUDE_GUARD_ADDRESS_LIST
#define TUNA_PRIV_INCLUDE_GUARD_ADDRESS_LIST

#include <tuna/error.h>
#include <tuna/ref.h>
#include <tuna/address.h>
#include <tuna/api.h>

#include <stddef.h>

///////////////////////////////////////////////////////////////////////////////

typedef struct tuna_address_list tuna_address_list;

TUNA_PRIV_API
tuna_error
tuna_get_address_list(tuna_ref const *ref, tuna_address_list **list_out);

TUNA_PRIV_API
void
tuna_free_address_list(tuna_address_list *list);

TUNA_PRIV_API
size_t
tuna_get_address_count(tuna_address_list const *list);

TUNA_PRIV_API
tuna_address
tuna_get_address_at(tuna_address_list const *list, size_t position);

///////////////////////////////////////////////////////////////////////////////

#endif
