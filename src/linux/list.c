#include <tuna/priv/linux.h>

#include <string.h>

///////////////////////////////////////////////////////////////////////////////

static
int
tuna_rtnl_link_cache_to_list(struct nl_cache *nl_cache,
                             size_t *count_out, tuna_ref **items)
{
    tuna_ref *ref = NULL;

    int err = 0;

    size_t count = 0;
    for (struct nl_object *nl_object = nl_cache_get_first(nl_cache);
         nl_object; nl_object = nl_cache_get_next(nl_object))
    {
        struct rtnl_link *rtnl_link = (void *)nl_object;

        char const *type = rtnl_link_get_type(rtnl_link);
        if (!type || strcmp(type, "tun")) {
            continue;
        }

        if (items) {
            if ((err = tuna_get_ref(&ref))) {
                goto out;
            }

            ref->index = rtnl_link_get_ifindex(rtnl_link);

            items[count] = ref; ref = NULL;
        }

        ++count;
    }

    if (count_out) {
        *count_out = count;
    }

out:
    tuna_free_ref(ref);

    return err;
}

TUNA_PRIV_API
tuna_error
tuna_get_list(tuna_list **list_out) {
    struct nl_sock *nl_sock = NULL;
    struct nl_cache *nl_cache = NULL;
    tuna_list *list = NULL;

    int err = 0;

    if ((err = tuna_open_nl_sock(&nl_sock))) {
        goto out;
    }

    if ((err = rtnl_link_alloc_cache(nl_sock, AF_UNSPEC, &nl_cache))) {
        err = tuna_translate_nl_error(-err);
        goto out;
    }

    size_t count = 0;
    if ((err = tuna_rtnl_link_cache_to_list(nl_cache, &count, NULL))
     || (err = tuna_allocate_list(count, &list))
     || (err = tuna_rtnl_link_cache_to_list(nl_cache, NULL, list->items)))
    { goto out; }

    *list_out = list; list = NULL;

out:
    tuna_free_list(list);
    nl_cache_free(nl_cache);
    nl_socket_free(nl_sock);

    return err;
}
