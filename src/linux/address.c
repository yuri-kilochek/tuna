#include <tuna/priv/linux.h>

///////////////////////////////////////////////////////////////////////////////

static
tuna_error
tuna_address_to_local_nl_addr(int index, tuna_address address,
                              struct rtnl_addr **rtnl_addr_out)
{
    struct rtnl_addr *rtnl_addr = NULL;
    struct nl_addr *nl_addr = NULL;

    int err = 0;

    if (!(rtnl_addr = rtnl_addr_alloc())) {
        err = TUNA_OUT_OF_MEMORY;
        goto out;
    }
    rtnl_addr_set_ifindex(rtnl_addr, index);

    switch (address.family) {
    case TUNA_IP4:
        if (!(nl_addr = nl_addr_build(AF_INET, address.ip4.bytes, 4))) {
            err = TUNA_OUT_OF_MEMORY;
            goto out;
        }
        nl_addr_set_prefixlen(nl_addr, address.ip4.prefix_length);
        break;
    case TUNA_IP6:
        if (!(nl_addr = nl_addr_build(AF_INET6, address.ip6.bytes, 16))) {
            err = TUNA_OUT_OF_MEMORY;
            goto out;
        }
        nl_addr_set_prefixlen(nl_addr, address.ip6.prefix_length);
        break;
    }
    if ((err = rtnl_addr_set_local(rtnl_addr, nl_addr))) {
        err = tuna_translate_nl_error(-err);
        goto out;
    }

    *rtnl_addr_out = rtnl_addr; rtnl_addr = NULL;

out:
    nl_addr_put(nl_addr);
    rtnl_addr_put(rtnl_addr);

    return err;
}

TUNA_PRIV_API
tuna_error
tuna_add_address(tuna_ref *ref, tuna_address address) {
    struct nl_sock *nl_sock = NULL;
    struct rtnl_addr *rtnl_addr = NULL;

    int err = 0;

    if ((err = tuna_open_nl_sock(&nl_sock))
     || (err = tuna_address_to_local_nl_addr(ref->index, address, &rtnl_addr)))
    { goto out; }

    if ((err = rtnl_addr_add(nl_sock, rtnl_addr, NLM_F_REPLACE))) {
        err = tuna_translate_nl_error(-err);
        goto out;
    }

out:
    rtnl_addr_put(rtnl_addr);
    nl_socket_free(nl_sock);

    return err;
}

TUNA_PRIV_API
tuna_error
tuna_remove_address(tuna_ref *ref, tuna_address address) {
    struct nl_sock *nl_sock = NULL;
    struct rtnl_addr *rtnl_addr = NULL;

    int err = 0;

    if ((err = tuna_open_nl_sock(&nl_sock))
     || (err = tuna_address_to_local_nl_addr(ref->index, address, &rtnl_addr)))
    { goto out; }

    if ((err = rtnl_addr_delete(nl_sock, rtnl_addr, 0))) {
        if (err == -NLE_NOADDR) {
            err = 0;
        } else {
            err = tuna_translate_nl_error(-err);
            goto out;
        }
    }

out:
    rtnl_addr_put(rtnl_addr);
    nl_socket_free(nl_sock);

    return err;
}
