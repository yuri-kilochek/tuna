#include <tuna/priv/linux/ref.h>
#include <tuna/ref.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if.h>
#include <linux/if_tun.h>

#include <netlink/route/addr.h>

#include <tuna/priv/string.h>
#include <tuna/priv/address_list.h>
#include <tuna/priv/linux/error.h>
#include <tuna/priv/linux/netlink.h>

///////////////////////////////////////////////////////////////////////////////

struct tuna_ref {
    unsigned int index;
    int fd;
};

tuna_error
tuna_allocate_ref(tuna_ref **ref_out) {
    tuna_ref *ref = NULL;

    tuna_error error = 0;

    if (!(ref = malloc(sizeof(*ref)))) {
        error = TUNA_ERROR_OUT_OF_MEMORY;
        goto out;
    }
    *ref = (tuna_ref) {
        .index = 0,
        .fd = -1,
    };

    *ref_out = ref; ref = NULL;
out:
    tuna_free_ref(ref);

    return error;
}

void
tuna_free_ref(tuna_ref *ref) {
    if (!ref) { return; }
    tuna_unbind(ref);
    free(ref);
}


void
tuna_unchecked_bind_by_index(tuna_ref *ref, unsigned int index) {
    tuna_unbind(ref);
    ref->index = index;
}

tuna_error
tuna_bind_like(tuna_ref *ref, tuna_ref const *example_ref) {
    tuna_unchecked_bind_by_index(ref, example_ref->index);
    return 0;
}

tuna_error
tuna_bind_by_index(tuna_ref *ref, tuna_index index) {
    char name[IF_NAMESIZE];
    if (!if_indextoname(index, name)) {
        if (errno == ENXIO) {
            return TUNA_ERROR_INTERFACE_NOT_FOUND;
        }
        return tuna_translate_errno();
    }

    tuna_unchecked_bind_by_index(ref, index);

    return 0;
}

tuna_error
tuna_bind_by_name(tuna_ref *ref, char const *name) {
    unsigned int index;
    if (!(index = if_nametoindex(name))) {
        if (errno == ENODEV) {
            return TUNA_ERROR_INTERFACE_NOT_FOUND;
        }
        return tuna_translate_errno();
    }

    tuna_unchecked_bind_by_index(ref, index);

    return 0;
}

tuna_bool
tuna_is_bound(tuna_ref const *ref) {
    return !!ref->index;
}

void
tuna_unbind(tuna_ref *ref) {
    tuna_close(ref);
    ref->index = 0;
}


tuna_error
tuna_get_index(tuna_ref const *ref, tuna_index *index_out) {
    *index_out = ref->index;

    return 0;
}


tuna_error
tuna_get_ownership(tuna_ref const *ref, tuna_ownership *ownership_out) {
    assert(tuna_is_bound(ref));

    struct nl_sock *sock = NULL;
    struct nl_msg *msg = NULL;

    tuna_error error = 0;
    int nlerr;

    if (0 > (nlerr = tuna_rtnl_socket_open(&sock))
     || 0 > (nlerr = tuna_rtnl_link_get_raw(sock, ref->index, &msg)))
    {
        error = tuna_translate_nlerr(nlerr);
        goto out;
    }

    struct nlattr *attr;
    if (!(attr = tuna_nlmsg_find_ifinfo_attr(msg, IFLA_LINKINFO))
     || !(attr = tuna_nla_find_nested(attr, IFLA_INFO_DATA))
     || !(attr = tuna_nla_find_nested(attr, IFLA_TUN_MULTI_QUEUE)))
    {
        error = TUNA_IMPL_ERROR_UNKNOWN;
        goto out;
    }

    *ownership_out = nla_get_u8(attr) ? TUNA_OWNERSHIP_SHARED
                                      : TUNA_OWNERSHIP_EXCLUSIVE;
out:
    nlmsg_free(msg);
    nl_socket_free(sock);

    return error;
}


tuna_error
tuna_get_lifetime(tuna_ref const *ref, tuna_lifetime *lifetime_out) {
    assert(tuna_is_bound(ref));

    struct nl_sock *sock = NULL;
    struct nl_msg *msg = NULL;

    tuna_error error = 0;
    int nlerr;

    if (0 > (nlerr = tuna_rtnl_socket_open(&sock))
     || 0 > (nlerr = tuna_rtnl_link_get_raw(sock, ref->index, &msg)))
    {
        error = tuna_translate_nlerr(nlerr);
        goto out;
    }

    struct nlattr *attr;
    if (!(attr = tuna_nlmsg_find_ifinfo_attr(msg, IFLA_LINKINFO))
     || !(attr = tuna_nla_find_nested(attr, IFLA_INFO_DATA))
     || !(attr = tuna_nla_find_nested(attr, IFLA_TUN_PERSIST)))
    {
        error = TUNA_IMPL_ERROR_UNKNOWN;
        goto out;
    }

    *lifetime_out = nla_get_u8(attr) ? TUNA_LIFETIME_PERSISTENT
                                     : TUNA_LIFETIME_TRANSIENT;
out:
    nlmsg_free(msg);
    nl_socket_free(sock);

    return error;
}

tuna_error
tuna_set_lifetime(tuna_ref *ref, tuna_lifetime lifetime) {
    assert(tuna_is_open(ref));

    unsigned long persist = (lifetime == TUNA_LIFETIME_PERSISTENT);
    if (ioctl(ref->fd, TUNSETPERSIST, persist) == -1) {
        return tuna_translate_errno();
    }

    return 0;
}


tuna_error
tuna_get_name(tuna_ref const *ref, char **name_out) {
    assert(tuna_is_bound(ref));

    char *name = NULL;

    tuna_error error = 0;

    if ((error = tuna_allocate_string(IF_NAMESIZE, &name))) {
        goto out;
    }
    if (!if_indextoname(ref->index, name)) {
        error = tuna_translate_errno();
        goto out;
    }

    *name_out = name; name = NULL;
out:
    tuna_free_string(name);

    return error;
}

tuna_error
tuna_set_name(tuna_ref *ref, char const *name) {
    assert(tuna_is_bound(ref));

    struct nl_sock *sock = NULL;
    struct rtnl_link *link = NULL;
    struct rtnl_link *link_changes = NULL;

    tuna_error error = 0;
    int nlerr;

    size_t length = strlen(name);
    if (length == 0) {
        error = TUNA_ERROR_INVALID_NAME;
        goto out;
    }
    if (length >= IF_NAMESIZE) {
        error = TUNA_ERROR_NAME_TOO_LONG;
        goto out;
    }

    if (0 > (nlerr = tuna_rtnl_socket_open(&sock))
     || 0 > (nlerr = rtnl_link_get_kernel(sock, ref->index, NULL, &link)))
    {
        error = tuna_translate_nlerr(nlerr);
        goto out;
    }

    if (!(link_changes = rtnl_link_alloc())) {
        error = TUNA_ERROR_OUT_OF_MEMORY;
        goto out;
    }
    rtnl_link_set_name(link_changes, name);

    if (0 > (nlerr = rtnl_link_change(sock, link, link_changes, 0))) {
        switch (nlerr) {
        case -NLE_INVAL:
            error = TUNA_ERROR_INVALID_NAME;
            goto out;
        case -NLE_EXIST:
            error = TUNA_ERROR_DUPLICATE_NAME;
            goto out;
        default:
            error = tuna_translate_nlerr(nlerr);
            goto out;
        }
    }
out:
    rtnl_link_put(link_changes);
    rtnl_link_put(link);
    nl_socket_free(sock);

    return error;
}


tuna_error
tuna_get_mtu(tuna_ref const *ref, size_t *mtu_out) {
    assert(tuna_is_bound(ref));

    struct nl_sock *sock = NULL;
    struct rtnl_link *link = NULL;

    tuna_error error = 0;
    int nlerr;

    if (0 > (nlerr = tuna_rtnl_socket_open(&sock))
     || 0 > (nlerr = rtnl_link_get_kernel(sock, ref->index, NULL, &link)))
    {
        error = tuna_translate_nlerr(nlerr);
        goto out;
    }

    *mtu_out = rtnl_link_get_mtu(link);
out:
    rtnl_link_put(link);
    nl_socket_free(sock);

    return error;
}

tuna_error
tuna_set_mtu(tuna_ref *ref, size_t mtu) {
    assert(tuna_is_bound(ref));

    struct nl_sock *sock = NULL;
    struct rtnl_link *link = NULL;
    struct rtnl_link *link_changes = NULL;

    tuna_error error = 0;
    int nlerr;

    if (0 > (nlerr = tuna_rtnl_socket_open(&sock))
     || 0 > (nlerr = rtnl_link_get_kernel(sock, ref->index, NULL, &link)))
    {
        error = tuna_translate_nlerr(nlerr);
        goto out;
    }
    size_t old_mtu = rtnl_link_get_mtu(link);

    if (!(link_changes = rtnl_link_alloc())) {
        error = TUNA_ERROR_OUT_OF_MEMORY;
        goto out;
    }
    rtnl_link_set_mtu(link_changes, mtu);

    if (0 > (nlerr = rtnl_link_change(sock, link, link_changes, 0))) {
        if (nlerr == -NLE_INVAL) {
            if (mtu < old_mtu) {
                error = TUNA_ERROR_MTU_TOO_SMALL;
                goto out;
            }
            if (mtu > old_mtu) {
                error = TUNA_ERROR_MTU_TOO_BIG;
                goto out;
            }
        }
        error = tuna_translate_nlerr(nlerr);
        goto out;
    }
out:
    rtnl_link_put(link_changes);
    rtnl_link_put(link);
    nl_socket_free(sock);

    return error;
}


static
void
tuna_translate_rtnl_addr_cache(unsigned int index, struct nl_cache *cache,
                               size_t *count_out, tuna_address *addresses)
{
    size_t count = 0;
    for (struct nl_object *object = nl_cache_get_first(cache);
         object; object = nl_cache_get_next(object))
    {
        struct rtnl_addr *addr = (void *) object;

        if ((unsigned int) rtnl_addr_get_ifindex(addr) != index) {
            continue;
        }

        struct nl_addr *local_addr = rtnl_addr_get_local(addr);
        switch (nl_addr_get_family(local_addr)) {
        case AF_INET:
            if (addresses) {
                tuna_ip4_address *ip4 = &addresses[count].ip4;
                ip4->family = TUNA_ADDRESS_FAMILY_IP4;
                memcpy(ip4->bytes, nl_addr_get_binary_addr(local_addr),
                       sizeof(ip4->bytes));
                ip4->prefix_length = nl_addr_get_prefixlen(local_addr);
            }
            break;
        case AF_INET6:
            if (addresses) {
                tuna_ip6_address *ip6 = &addresses[count].ip6;
                ip6->family = TUNA_ADDRESS_FAMILY_IP6;
                memcpy(ip6->bytes, nl_addr_get_binary_addr(local_addr),
                       sizeof(ip6->bytes));
                ip6->prefix_length = nl_addr_get_prefixlen(local_addr);
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

tuna_error
tuna_get_addresses(tuna_ref const *ref, tuna_address_list **list_out) {
    assert(tuna_is_bound(ref));

    struct nl_sock *sock = NULL;
    struct nl_cache *cache = NULL;
    tuna_address_list *list = NULL;

    tuna_error error = 0;
    int nlerr;

    if (0 > (nlerr = tuna_rtnl_socket_open(&sock))
     || 0 > (nlerr = rtnl_addr_alloc_cache(sock, &cache)))
    {
        error = tuna_translate_nlerr(nlerr);
        goto out;
    }

    size_t count = 0;
    tuna_translate_rtnl_addr_cache(ref->index, cache, &count, NULL);
    if ((error = tuna_allocate_address_list(count, &list))) {
        goto out;
    }
    tuna_translate_rtnl_addr_cache(ref->index, cache, NULL, list->items);

    *list_out = list; list = NULL;
out:
    tuna_free_address_list(list);
    nl_cache_free(cache);
    nl_socket_free(sock);

    return error;
}

static
tuna_error
tuna_translate_address(unsigned int index,
                       tuna_address const *address,
                       struct rtnl_addr **addr_out)
{
    struct rtnl_addr *addr = NULL;
    struct nl_addr *local_addr = NULL;

    tuna_error error = 0;
    int nlerr;

    if (!(addr = rtnl_addr_alloc())) {
        error = TUNA_ERROR_OUT_OF_MEMORY;
        goto out;
    }
    rtnl_addr_set_ifindex(addr, index);

    switch (address->family) {
    case TUNA_ADDRESS_FAMILY_IP4:;
        tuna_ip4_address const *ip4 = &address->ip4;
        if (!(local_addr = nl_addr_build(AF_INET,
                                         ip4->bytes, sizeof(ip4->bytes))))
        {
            error = TUNA_ERROR_OUT_OF_MEMORY;
            goto out;
        }
        nl_addr_set_prefixlen(local_addr, ip4->prefix_length);
        break;
    case TUNA_ADDRESS_FAMILY_IP6:;
        tuna_ip6_address const *ip6 = &address->ip6;
        if (!(local_addr = nl_addr_build(AF_INET6,
                                         ip6->bytes, sizeof(ip6->bytes))))
        {
            error = TUNA_ERROR_OUT_OF_MEMORY;
            goto out;
        }
        nl_addr_set_prefixlen(local_addr, ip6->prefix_length);
        break;
    default:
        error = TUNA_ERROR_UNKNOWN_ADDRESS_FAMILY;
        goto out;
    }
    if (0 > (nlerr = rtnl_addr_set_local(addr, local_addr))) {
        error = tuna_translate_nlerr(nlerr);
        goto out;
    }

    *addr_out = addr; addr = NULL;
out:
    nl_addr_put(local_addr);
    rtnl_addr_put(addr);

    return error;
}

tuna_error
tuna_add_address(tuna_ref *ref, tuna_address const *address) {
    assert(tuna_is_bound(ref));

    struct nl_sock *sock = NULL;
    struct rtnl_addr *addr = NULL;

    tuna_error error = 0;
    int nlerr;

    if (0 > (nlerr = tuna_rtnl_socket_open(&sock))) {
        error = tuna_translate_nlerr(nlerr);
        goto out;
    }

    if ((error = tuna_translate_address(ref->index, address, &addr))) {
        goto out;
    }

    if (0 > (nlerr = rtnl_addr_add(sock, addr, NLM_F_REPLACE))) {
        error = tuna_translate_nlerr(nlerr);
        goto out;
    }
out:
    rtnl_addr_put(addr);
    nl_socket_free(sock);

    return error;
}

tuna_error
tuna_remove_address(tuna_ref *ref, tuna_address const *address) {
    assert(tuna_is_bound(ref));

    struct nl_sock *sock = NULL;
    struct rtnl_addr *addr = NULL;

    tuna_error error = 0;
    int nlerr;

    if (0 > (nlerr = tuna_rtnl_socket_open(&sock))) {
        error = tuna_translate_nlerr(nlerr);
        goto out;
    }

    if ((error = tuna_translate_address(ref->index, address, &addr))) {
        goto out;
    }

    if (0 > (nlerr = rtnl_addr_delete(sock, addr, 0))) {
        if (nlerr != -NLE_NOADDR) {
            error = tuna_translate_nlerr(nlerr);
            goto out;
        }
    }
out:
    rtnl_addr_put(addr);
    nl_socket_free(sock);

    return error;
}


tuna_error
tuna_get_admin_state(tuna_ref const *ref, tuna_admin_state *state_out) {
    assert(tuna_is_bound(ref));

    struct nl_sock *sock = NULL;
    struct rtnl_link *link = NULL;

    tuna_error error = 0;
    int nlerr;

    if (0 > (nlerr = tuna_rtnl_socket_open(&sock))
     || 0 > (nlerr = rtnl_link_get_kernel(sock, ref->index, NULL, &link)))
    {
        error = tuna_translate_nlerr(nlerr);
        goto out;
    }

    if (rtnl_link_get_flags(link) & IFF_UP) {
        *state_out = TUNA_ADMIN_STATE_ENABLED;
    } else {
        *state_out = TUNA_ADMIN_STATE_DISABLED;
    }
out:
    rtnl_link_put(link);
    nl_socket_free(sock);

    return error;
}

tuna_error
tuna_set_admin_state(tuna_ref *ref, tuna_admin_state state) {
    assert(tuna_is_bound(ref));

    struct nl_sock *sock = NULL;
    struct rtnl_link *link = NULL;
    struct rtnl_link *link_changes = NULL;

    tuna_error error = 0;
    int nlerr;

    if (0 > (nlerr = tuna_rtnl_socket_open(&sock))
     || 0 > (nlerr = rtnl_link_get_kernel(sock, ref->index, NULL, &link)))
    {
        error = tuna_translate_nlerr(nlerr);
        goto out;
    }

    if (!(link_changes = rtnl_link_alloc())) {
        error = TUNA_ERROR_OUT_OF_MEMORY;
        goto out;
    }
    if (state == TUNA_ADMIN_STATE_ENABLED) {
        rtnl_link_set_flags(link_changes, IFF_UP);
    } else {
        rtnl_link_unset_flags(link_changes, IFF_UP);
    }

    if (0 > (nlerr = rtnl_link_change(sock, link, link_changes, 0))) {
        error = tuna_translate_nlerr(nlerr);
        goto out;
    }
out:
    rtnl_link_put(link_changes);
    rtnl_link_put(link);
    nl_socket_free(sock);

    return error;
}


tuna_error
tuna_get_link_state(tuna_ref const *ref, tuna_link_state *state_out) {
    assert(tuna_is_bound(ref));

    struct nl_sock *sock = NULL;
    struct rtnl_link *link = NULL;

    tuna_error error = 0;
    int nlerr;

    if (0 > (nlerr = tuna_rtnl_socket_open(&sock))
     || 0 > (nlerr = rtnl_link_get_kernel(sock, ref->index, NULL, &link)))
    {
        error = tuna_translate_nlerr(nlerr);
        goto out;
    }

    *state_out = rtnl_link_get_carrier(link) ? TUNA_LINK_STATE_CONNECTED
                                             : TUNA_LINK_STATE_DISCONNECTED;
out:
    rtnl_link_put(link);
    nl_socket_free(sock);

    return error;
}

tuna_error
tuna_set_link_state(tuna_ref *ref, tuna_link_state state) {
    assert(tuna_is_open(ref));

    unsigned int carrier = (state == TUNA_LINK_STATE_CONNECTED);
    if (ioctl(ref->fd, TUNSETCARRIER, carrier) == -1) {
        return tuna_translate_errno();
    }

    return 0;
}


tuna_error
tuna_open(tuna_ref *ref, tuna_ownership ownership) {
    assert(!tuna_is_open(ref));

    struct nl_sock *sock = NULL;
    struct rtnl_link *link = NULL;
    int fd = -1;

    tuna_error error = 0;
    int nlerr;

    if (0 > (nlerr = tuna_rtnl_socket_open(&sock))) {
        error = tuna_translate_nlerr(nlerr);
        goto out;
    }

    struct ifreq ifr = {
        .ifr_flags = IFF_TUN | IFF_NO_PI,
    };
    if (ownership == TUNA_OWNERSHIP_SHARED) {
        ifr.ifr_flags |= IFF_MULTI_QUEUE;
    }
    if (tuna_is_bound(ref)) {
        if (0 > (nlerr = rtnl_link_get_kernel(sock, ref->index, NULL, &link)))
        {
            error = tuna_translate_nlerr(nlerr);
            goto out;
        }

        strcpy(ifr.ifr_name, rtnl_link_get_name(link));
    }

    if ((fd = open("/dev/net/tun", O_RDWR)) == -1
     || ioctl(fd, TUNSETIFF, &ifr) == -1)
    {
        error = tuna_translate_errno();
        goto out;
    }

    // XXX: Since we only get the interface's name that can be changed by any
    // priviledged process and there appears to be no `TUNGETIFINDEX` or other
    // machanism to obtain interface index directly from file descriptor,
    // here we have a potential race condition.
get_index:
    if (ioctl(nl_socket_get_fd(sock), SIOCGIFINDEX, &ifr) == -1) {
        // XXX: The following handles the case when it's just this interface
        // that has been renamed, however if the freed name has then been
        // assigned to another interface, then we'll incorrectly get that
        // interface's index and no error. We can then attempt to refetch the
        // name via `TUNGETIFF` and check if they match, however it is possible
        // that before we can do that, our interface is renamed back to its
        // original name, causing the check to incorrectly succeed. Since there
        // appears to be no way to completely rule out a race, we content
        // ourselves with only handling the basic case, judging the rest
        // extremely unlikely to ever happen.
        if (errno == ENODEV && ioctl(fd, TUNGETIFF, &ifr) != -1) {
            goto get_index;
        }
        error = tuna_translate_errno();
        goto out;
    }
    unsigned int index = ifr.ifr_ifindex;

    // Even assuming the interface's index has been determined correctly, if
    // the interface has been deleted or renamed just before `TUNSETIFF`, then
    // `TUNSETIFF` will succeed and create a new interface instead of attaching
    // to an existing one. At least we can detect this afterwards, since the
    // new interface will have a different index.
    if (tuna_is_bound(ref) && index != ref->index) {
        error = TUNA_ERROR_INTERFACE_LOST;
        goto out;
    }

    ref->index = index;
    ref->fd = fd; fd = -1;
out:
    if (fd != -1) { close(fd); }
    rtnl_link_put(link);
    nl_socket_free(sock);

    return error;
}

tuna_bool
tuna_is_open(tuna_ref const *ref) {
    return ref->fd != -1;
}

void
tuna_close(tuna_ref *ref) {
    if (ref->fd != -1) {
        close(ref->fd); ref->fd = -1;
    }
}


tuna_handle
tuna_get_handle(tuna_ref const *ref) {
    return ref->fd;
}

