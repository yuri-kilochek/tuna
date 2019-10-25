#include <tuna/priv/linux.h>

#include <assert.h>

///////////////////////////////////////////////////////////////////////////////

TUNA_PRIV_API
tuna_error
tuna_set_mtu(tuna_ref *ref, size_t mtu) {
    assert(tuna_is_bound(ref));

    struct nl_sock *nl_sock = NULL;
    struct rtnl_link *rtnl_link = NULL;
    struct rtnl_link *rtnl_link_changes = NULL;

    int err = 0;

    if ((err = tuna_open_nl_sock(&nl_sock))
     || (err = tuna_get_rtnl_link_via(nl_sock, ref->index, &rtnl_link))
     || (err = tuna_allocate_rtnl_link(&rtnl_link_changes)))
    { goto out; }

    rtnl_link_set_mtu(rtnl_link_changes, mtu);

    if ((err = tuna_change_rtnl_link_via(nl_sock,
                                         rtnl_link, rtnl_link_changes)))
    {
        if (err == -NLE_INVAL) {
            size_t current_mtu = rtnl_link_get_mtu(rtnl_link);
            if (mtu < current_mtu) {
                err = TUNA_MTU_TOO_SMALL;
                goto out;
            }
            if (mtu > current_mtu) {
                err = TUNA_MTU_TOO_BIG;
                goto out;
            }
        }
        err = tuna_translate_nl_error(-err);
        goto out;
    }

out:
    rtnl_link_put(rtnl_link_changes);
    rtnl_link_put(rtnl_link);
    nl_socket_free(nl_sock);

    return err;
}

TUNA_PRIV_API
tuna_error
tuna_get_mtu(tuna_ref const *ref, size_t *mtu_out) {
    assert(tuna_is_bound(ref));

    struct rtnl_link *rtnl_link = NULL;

    int err = 0;

    if ((err = tuna_get_rtnl_link(ref->index, &rtnl_link))) {
        goto out;
    }

    *mtu_out = rtnl_link_get_mtu(rtnl_link);

out:
    rtnl_link_put(rtnl_link);

    return err;
}
