#ifndef TUNA_IMPL_INCL_ADDRESS_LIST_H
#define TUNA_IMPL_INCL_ADDRESS_LIST_H

#include <stddef.h>

#include <tuna/address.h>

#include <tuna/impl/prolog.inc>
///////////////////////////////////////////////////////////////////////////////

typedef struct tuna_address_list tuna_address_list;

TUNA_IMPL_API
size_t
tuna_get_address_count(tuna_address_list const *list);

TUNA_IMPL_API
tuna_address const *
tuna_get_address_at(tuna_address_list const *list, size_t position);

TUNA_IMPL_API
void
tuna_free_address_list(tuna_address_list *list);

///////////////////////////////////////////////////////////////////////////////
#include <tuna/impl/epilog.inc>

#endif

