#ifndef TUNA_IMPL_INCL_PRIV_ADDRESS_LIST_H
#define TUNA_IMPL_INCL_PRIV_ADDRESS_LIST_H

#include <stddef.h>

#include <tuna/address_list.h>
#include <tuna/address.h>
#include <tuna/error.h>

///////////////////////////////////////////////////////////////////////////////

struct tuna_address_list {
    size_t count;
    tuna_address items[];
};

tuna_error
tuna_allocate_address_list(size_t count, tuna_address_list **list_out);

///////////////////////////////////////////////////////////////////////////////

#endif

