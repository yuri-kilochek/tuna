#include <tuna/priv/linux.h>

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <unistd.h>
#include <net/if.h>
#include <linux/if.h>

///////////////////////////////////////////////////////////////////////////////

TUNA_PRIV_API
tuna_error
tuna_set_name(tuna_ref *ref, char const *name) {
    assert(tuna_is_bound(ref));

    struct nl_sock *nl_sock = NULL;
    struct rtnl_link *rtnl_link = NULL;
    struct rtnl_link *rtnl_link_changes = NULL;
    unsigned flags = 0;

    int err = 0;

    size_t length = strlen(name);
    if (((length == 0) && (err = TUNA_INVALID_NAME))
     || ((length >= IF_NAMESIZE) && (err = TUNA_NAME_TOO_LONG))
     || (err = tuna_open_nl_sock(&nl_sock))
     || (err = tuna_change_rtnl_link_flags_via(nl_sock, ref->index,
                                               rtnl_link_unset_flags,
                                               IFF_UP, &flags))
     || (err = tuna_get_rtnl_link_via(nl_sock, ref->index, &rtnl_link))
     || (err = tuna_allocate_rtnl_link(&rtnl_link_changes)))
    { goto out; }

    rtnl_link_set_name(rtnl_link_changes, name);

    if ((err = tuna_change_rtnl_link_via(nl_sock,
                                         rtnl_link, rtnl_link_changes)))
    {
        switch (err) {
        case -NLE_INVAL:
            err = TUNA_INVALID_NAME;
            break;
        case -NLE_EXIST:
            err = TUNA_DUPLICATE_NAME;
            break;
        default:
            err = tuna_translate_nl_error(-err);
        }
        goto out;
    }

out:
    rtnl_link_put(rtnl_link_changes);
    rtnl_link_put(rtnl_link);

    if (tuna_change_rtnl_link_flags_via(nl_sock, ref->index,
                                        rtnl_link_set_flags,
                                        flags & IFF_UP, NULL))
    { err = TUNA_DEVICE_LOST; }

    nl_socket_free(nl_sock);

    return err;
}

TUNA_PRIV_API
tuna_error
tuna_get_name(tuna_ref const *ref, char **name_out) {
    assert(tuna_is_bound(ref));

    char *name = NULL;

    int err = 0;

    if (!(name = malloc(IF_NAMESIZE))
     || !if_indextoname(ref->index, name))
    {
        err = tuna_translate_sys_error(errno);
        goto out;
    }

    *name_out = name; name = NULL;

out:
    tuna_free_name(name);

    return err;
}

TUNA_PRIV_API
void
tuna_free_name(char *name) {
    free(name);
}

TUNA_PRIV_API
tuna_error
tuna_bind_by_name(tuna_ref *ref, char const *name) {
    unsigned index;
    if (!(index = if_nametoindex(name))) {
        if (errno == ENODEV) {
            return TUNA_DEVICE_NOT_FOUND;
        }
        return tuna_translate_sys_error(errno);
    }

    tuna_unchecked_bind_by_index(ref, index);

    return 0;
}

