#ifndef TUNA_IMPL_INCL_PRIV_REF_LIST_H
#define TUNA_IMPL_INCL_PRIV_REF_LIST_H

#include <stddef.h>

#include <tuna/ref_list.h>
#include <tuna/ref.h>
#include <tuna/error.h>

///////////////////////////////////////////////////////////////////////////////

struct tuna_ref_list {
    size_t count;
    tuna_ref *items[];
};

tuna_error
tuna_allocate_ref_list(size_t count, tuna_ref_list **list_out);

///////////////////////////////////////////////////////////////////////////////

#endif

