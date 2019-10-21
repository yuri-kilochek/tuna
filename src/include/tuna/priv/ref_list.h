#ifndef TUNA_PRIV_INCLUDE_GUARD_PRIV_REF_LIST
#define TUNA_PRIV_INCLUDE_GUARD_PRIV_REF_LIST

#include <tuna.h>

#include <stddef.h>

///////////////////////////////////////////////////////////////////////////////

struct tuna_ref_list {
    size_t count;
    tuna_ref *items[];
};

int
tuna_allocate_ref_list(size_t count, tuna_ref_list **list_out);

///////////////////////////////////////////////////////////////////////////////

#endif
