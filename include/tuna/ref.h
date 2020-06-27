#ifndef TUNA_IMPL_INCL_REF_H
#define TUNA_IMPL_INCL_REF_H

#include <stddef.h>

#include <tuna/error.h>
#include <tuna/index.h>
#include <tuna/ownership.h>
#include <tuna/lifetime.h>
#include <tuna/string.h>
#include <tuna/address_list.h>
#include <tuna/address.h>
#include <tuna/admin_state.h>
#include <tuna/link_state.h>
#include <tuna/bool.h>
#include <tuna/handle.h>

#include <tuna/impl/prolog.inc>
///////////////////////////////////////////////////////////////////////////////

typedef struct tuna_ref tuna_ref;


TUNA_IMPL_API
tuna_error
tuna_allocate_ref(tuna_ref **ref_out);

TUNA_IMPL_API
void
tuna_free_ref(tuna_ref *ref);


TUNA_IMPL_API
tuna_error 
tuna_bind_like(tuna_ref *ref, tuna_ref const *example_ref);

TUNA_IMPL_API
tuna_error 
tuna_bind_by_index(tuna_ref *ref, tuna_index index);

TUNA_IMPL_API
tuna_error 
tuna_bind_by_name(tuna_ref *ref, char const *name);

TUNA_IMPL_API
tuna_bool
tuna_is_bound(tuna_ref const *ref);

TUNA_IMPL_API
void
tuna_unbind(tuna_ref *ref);


TUNA_IMPL_API
tuna_error
tuna_get_index(tuna_ref const *ref, tuna_index *index_out);


TUNA_IMPL_API
tuna_error 
tuna_get_ownership(tuna_ref const *ref, tuna_ownership *ownership_out);


TUNA_IMPL_API
tuna_error
tuna_get_lifetime(tuna_ref const *ref, tuna_lifetime *lifetime_out);

TUNA_IMPL_API
tuna_error 
tuna_set_lifetime(tuna_ref *ref, tuna_lifetime lifetime);


TUNA_IMPL_API
tuna_error
tuna_get_name(tuna_ref const *ref, char **name_out);

TUNA_IMPL_API
tuna_error 
tuna_set_name(tuna_ref *ref, char const *name);


TUNA_IMPL_API
tuna_error
tuna_get_mtu(tuna_ref const *ref, size_t *mtu_out);

TUNA_IMPL_API
tuna_error 
tuna_set_mtu(tuna_ref *ref, size_t mtu);


TUNA_IMPL_API
tuna_error
tuna_get_addresses(tuna_ref const *ref, tuna_address_list **list_out);

TUNA_IMPL_API
tuna_error 
tuna_add_address(tuna_ref *ref, tuna_address const *address);

TUNA_IMPL_API
tuna_error 
tuna_remove_address(tuna_ref *ref, tuna_address const *address);


TUNA_IMPL_API
tuna_error 
tuna_get_admin_state(tuna_ref const *ref, tuna_admin_state *state_out);

TUNA_IMPL_API
tuna_error 
tuna_set_admin_state(tuna_ref *ref, tuna_admin_state state);


TUNA_IMPL_API
tuna_error 
tuna_get_link_state(tuna_ref const *ref, tuna_link_state *state_out);

TUNA_IMPL_API
tuna_error 
tuna_set_link_state(tuna_ref *ref, tuna_link_state state);


TUNA_IMPL_API
tuna_error 
tuna_open(tuna_ref *ref, tuna_ownership ownership);

TUNA_IMPL_API
tuna_bool
tuna_is_open(tuna_ref const *ref);

TUNA_IMPL_API
void
tuna_close(tuna_ref *ref);


TUNA_IMPL_API
tuna_handle
tuna_get_handle(tuna_ref const *ref);

///////////////////////////////////////////////////////////////////////////////
#include <tuna/impl/epilog.inc>

#endif

