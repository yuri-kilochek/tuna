#ifndef TUNA_PRIV_INCLUDE_GUARD_PRIV_LIST
#define TUNA_PRIV_INCLUDE_GUARD_PRIV_LIST

#include <tuna.h>

#include <stddef.h>

///////////////////////////////////////////////////////////////////////////////

struct tuna_list {
    size_t count;
    tuna_ref *items[];
};

int
tuna_allocate_list(size_t count, tuna_list **list_out);

///////////////////////////////////////////////////////////////////////////////

#endif
