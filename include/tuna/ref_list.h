#ifndef TUNA_IMPL_INCL_REF_LIST_H
#define TUNA_IMPL_INCL_REF_LIST_H

#include <stddef.h>

#include <tuna/ref.h>

#include <tuna/impl/prolog.inc>
///////////////////////////////////////////////////////////////////////////////

typedef struct tuna_ref_list tuna_ref_list;

TUNA_IMPL_API
tuna_error
tuna_get_refs(tuna_ref_list **list_out);

TUNA_IMPL_API
size_t
tuna_get_ref_count(tuna_ref_list const *list);

TUNA_IMPL_API
tuna_ref const *
tuna_get_ref_at(tuna_ref_list const *list, size_t position);

TUNA_IMPL_API
void
tuna_free_ref_list(tuna_ref_list *list);

///////////////////////////////////////////////////////////////////////////////
#include <tuna/impl/epilog.inc>

#endif

