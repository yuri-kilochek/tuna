#include <tuna/priv/linux.h>

#include <netlink/route/addr.h>

#include <stddef.h>
#include <string.h>
#include <assert.h>

///////////////////////////////////////////////////////////////////////////////

static
void
tuna_rtnl_addr_cache_to_address_list(int index, struct nl_cache *nl_cache,
                                     size_t *count_out,
                                     tuna_address *addresses)
{
    size_t count = 0;
    for (struct nl_object *nl_object = nl_cache_get_first(nl_cache);
         nl_object; nl_object = nl_cache_get_next(nl_object))
    {
        struct rtnl_addr *rtnl_addr = (void *)nl_object;

        if (rtnl_addr_get_ifindex(rtnl_addr) != index) {
            continue;
        }

        struct nl_addr *nl_addr = rtnl_addr_get_local(rtnl_addr);
        switch (nl_addr_get_family(nl_addr)) {
        case AF_INET:
            if (addresses) {
                tuna_ip4_address *ip4 = &addresses[count].ip4;
                ip4->family = TUNA_IP4;
                memcpy(ip4->bytes, nl_addr_get_binary_addr(nl_addr), 4);
                ip4->prefix_length = nl_addr_get_prefixlen(nl_addr);
            }
            break;
        case AF_INET6:
            if (addresses) {
                tuna_ip6_address *ip6 = &addresses[count].ip6;
                ip6->family = TUNA_IP6;
                memcpy(ip6->bytes, nl_addr_get_binary_addr(nl_addr), 16);
                ip6->prefix_length = nl_addr_get_prefixlen(nl_addr);
            }
            break;
        default:
            continue;
        }

        ++count;
    }

    if (count_out) {
        *count_out = count;
    }
}

TUNA_PRIV_API
tuna_error
tuna_get_address_list(tuna_ref const *ref, tuna_address_list **list_out) {
    assert(tuna_is_bound(ref));

    struct nl_sock *nl_sock = NULL;
    struct nl_cache *nl_cache = NULL;
    tuna_address_list *list = NULL;

    int err = 0;

    if ((err = tuna_open_nl_sock(&nl_sock))) {
        goto out;
    }

    if ((err = rtnl_addr_alloc_cache(nl_sock, &nl_cache))) {
        err = tuna_translate_nl_error(-err);
        goto out;
    }

    size_t count = 0;
    if ((tuna_rtnl_addr_cache_to_address_list(ref->index, nl_cache,
                                              &count, NULL), 0)
     || (err = tuna_allocate_address_list(count, &list))
     || (tuna_rtnl_addr_cache_to_address_list(ref->index, nl_cache,
                                              NULL, list->items), 0))
    { goto out; }

    *list_out = list; list = NULL;

out:
    tuna_free_address_list(list);
    nl_cache_free(nl_cache);
    nl_socket_free(nl_sock);

    return err;
}
