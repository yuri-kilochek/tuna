#ifndef TUNA_PRIV_INCLUDE_GUARD_PRIV_ADDRESS_LIST
#define TUNA_PRIV_INCLUDE_GUARD_PRIV_ADDRESS_LIST

#include <tuna.h>

#include <stddef.h>

///////////////////////////////////////////////////////////////////////////////

struct tuna_address_list {
    size_t count;
    tuna_address items[];
};

int
tuna_allocate_address_list(size_t count, tuna_address_list **list_out);

///////////////////////////////////////////////////////////////////////////////

#endif
