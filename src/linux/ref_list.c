#include <tuna/priv/ref_list.h>
#include <tuna/ref_list.h>

#include <string.h>

#include <netlink/route/link.h>

#include <tuna/priv/linux/netlink.h>
#include <tuna/priv/linux/ref.h>

///////////////////////////////////////////////////////////////////////////////

static
void
tuna_translate_rtnl_link_cache(struct nl_cache *cache,
                               size_t *count_out, tuna_ref **refs)
{
    size_t count = 0;
    for (struct nl_object *object = nl_cache_get_first(cache);
         object; object = nl_cache_get_next(object))
    {
        struct rtnl_link *link = (void *) object;

        char const *type = rtnl_link_get_type(link);
        if (!type || strcmp(type, "tun")) {
            continue;
        }

        if (refs) {
            tuna_unchecked_bind_by_index(refs[count],
                                         rtnl_link_get_ifindex(link));
        }

        ++count;
    }

    if (count_out) {
        *count_out = count;
    }
}

tuna_error
tuna_get_refs(tuna_ref_list **list_out) {
    struct nl_sock *sock = NULL;
    struct nl_cache *cache = NULL;
    tuna_ref_list *list = NULL;

    tuna_error error = 0;
    int nlerr;

    if (0 > (nlerr = tuna_rtnl_socket_open(&sock))
     || 0 > (nlerr = rtnl_link_alloc_cache(sock, AF_UNSPEC, &cache)))
    {
        error = tuna_translate_nlerr(nlerr);
        goto out;
    }

    size_t count = 0;
    tuna_translate_rtnl_link_cache(cache, &count, NULL);
    if ((error = tuna_allocate_ref_list(count, &list))) {
        goto out;
    }
    tuna_translate_rtnl_link_cache(cache, NULL, list->items);

    *list_out = list; list = NULL;
out:
    tuna_free_ref_list(list);
    nl_cache_free(cache);
    nl_socket_free(sock);

    return error;
}

