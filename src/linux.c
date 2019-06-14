#include <tuna.h>

#include <errno.h>
#include <string.h>
#include <assert.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if.h>
#include <linux/if_tun.h>

#include <netlink/socket.h>
#include <netlink/route/link.h>
#include <netlink/route/addr.h>

/// TODO: remove
#include <stdio.h>

///////////////////////////////////////////////////////////////////////////////

struct tuna_device {
    int index;
    int fd;
};

static
void
tuna_finalize_device(tuna_device *device) {
    if (device->fd != -1) {
        close(device->fd);
    }
}

void
tuna_close_device(tuna_device *device) {
    if (device) {
        tuna_finalize_device(device);

        free(device);
    }
}

static
tuna_error
tuna_translate_syserr(int err) {
    switch (err) {
    case 0:
        return 0;
    case EPERM:
        return TUNA_FORBIDDEN;
    case ENOMEM:
    case ENOBUFS:
        return TUNA_OUT_OF_MEMORY;
    case EMFILE:
    case ENFILE:
        return TUNA_TOO_MANY_HANDLES;
    case ENXIO:
    case ENODEV:
        return TUNA_DEVICE_LOST;
    case EBUSY:
        return TUNA_DEVICE_BUSY;
    default:
        return TUNA_UNEXPECTED;
    }
}

static
tuna_error
tuna_translate_nlerr(int err) {
    switch (err) {
    case 0:
        return 0;
    case NLE_PERM:
        return TUNA_FORBIDDEN;
    case NLE_NOMEM:
        return TUNA_OUT_OF_MEMORY;
    case NLE_NODEV:
    case NLE_OBJ_NOTFOUND:
        return TUNA_DEVICE_LOST;
    default:
        return TUNA_UNEXPECTED;
    }
}

static
tuna_error
tuna_open_nl_sock(struct nl_sock **nl_sock_out) {
    struct nl_sock *nl_sock = NULL;

    int err = 0;

    errno = 0;
    if (!(nl_sock = nl_socket_alloc())) {
        if (errno) {
            err = tuna_translate_syserr(errno);
        } else {
            err = TUNA_OUT_OF_MEMORY;
        }
        goto out;
    }

    errno = 0;
    if ((err = nl_connect(nl_sock, NETLINK_ROUTE))) {
        if (err == -NLE_FAILURE) {
            switch (errno) {
            case EMFILE:
            case ENFILE:
                err = TUNA_TOO_MANY_HANDLES;
                goto out;
            }
        }
        err = tuna_translate_nlerr(-err);
        goto out;
    }

    *nl_sock_out = nl_sock; nl_sock = NULL;

out:
    nl_socket_free(nl_sock);

    return err;
}

static
tuna_error
tuna_query_nl_link(int index, nl_recvmsg_msg_cb_t callback, void *context) {
    struct nl_sock *nl_sock = NULL;
    struct nl_msg *nl_msg = NULL;

    int err = 0;

    if ((err = tuna_open_nl_sock(&nl_sock))) {
        goto out;
    }

    if ((err = nl_socket_modify_cb(nl_sock,
                                   NL_CB_VALID, NL_CB_CUSTOM,
                                   callback, context)))
    {
        err = tuna_translate_nlerr(-err);
        goto out;
    }

    if ((err = rtnl_link_build_get_request(index, NULL, &nl_msg))) {
        err = tuna_translate_nlerr(-err);
        goto out;
    }

    if ((err = nl_send_auto(nl_sock, nl_msg)) < 0) {
        err = tuna_translate_nlerr(-err);
        goto out;
    }
    err = 0;

    if ((err = nl_recvmsgs_default(nl_sock))) {
        err = tuna_translate_nlerr(-err);
        goto out;
    }

out:
    nlmsg_free(nl_msg);
    nl_socket_free(nl_sock);

    return err;
}

static
struct nlattr *
tuna_resolve_nlattr(struct nl_msg *nl_msg, int hdrlen, int depth, int *types) {
    if (depth == 0) { return NULL; }
    struct nlattr *nlattr = 
        nlmsg_find_attr(nlmsg_hdr(nl_msg), hdrlen, types[0]);
    for (int i = 1; nlattr && i < depth; ++i) {
        nlattr = nla_find(nla_data(nlattr), nla_len(nlattr), types[i]);
    }
    return nlattr;
}

#define TUNA_ARRAY_SIZE(...) \
    (sizeof(__VA_ARGS__) / sizeof(*(__VA_ARGS__))) \
/**/

static
int
tuna_get_ownership_callback(struct nl_msg *nl_msg, void *context) {
    tuna_ownership *ownership = context;

    int path[] = {IFLA_LINKINFO, IFLA_INFO_DATA, IFLA_TUN_MULTI_QUEUE};
    struct nlattr *nlattr =
        tuna_resolve_nlattr(nl_msg, sizeof(struct ifinfomsg),
                            TUNA_ARRAY_SIZE(path), path);

    if (nla_get_u8(nlattr)) {
        *ownership = TUNA_SHARED;
    } else {
        *ownership = TUNA_EXCLUSIVE;
    }

    return NL_STOP;
}

tuna_error
tuna_get_ownership(tuna_device const *device, tuna_ownership *ownership) {
    if (device->fd == -1) {
        return tuna_query_nl_link(device->index,
                                  tuna_get_ownership_callback, ownership);
    }

    struct ifreq ifr;
    if (ioctl(device->fd, TUNGETIFF, &ifr) == -1) {
        return tuna_translate_syserr(errno);
    }

    if (ifr.ifr_flags & IFF_MULTI_QUEUE) {
        *ownership = TUNA_SHARED;
    } else {
        *ownership = TUNA_EXCLUSIVE;
    }

    return 0;
}

static
int
tuna_get_lifetime_callback(struct nl_msg *nl_msg, void *context) {
    tuna_lifetime *lifetime = context;

    int path[] = {IFLA_LINKINFO, IFLA_INFO_DATA, IFLA_TUN_PERSIST};
    struct nlattr *nlattr =
        tuna_resolve_nlattr(nl_msg, sizeof(struct ifinfomsg),
                            TUNA_ARRAY_SIZE(path), path);

    if (nla_get_u8(nlattr)) {
        *lifetime = TUNA_PERSISTENT;
    } else {
        *lifetime = TUNA_TRANSIENT;
    }

    return NL_STOP;
}

tuna_error
tuna_get_lifetime(tuna_device const *device, tuna_lifetime *lifetime) {
    if (device->fd == -1) {
        return tuna_query_nl_link(device->index,
                                  tuna_get_lifetime_callback, lifetime);
    }

    struct ifreq ifr;
    if (ioctl(device->fd, TUNGETIFF, &ifr) == -1) {
        return tuna_translate_syserr(errno);
    }

    if (ifr.ifr_flags & IFF_PERSIST) {
        *lifetime = TUNA_PERSISTENT;
    } else {
        *lifetime = TUNA_TRANSIENT;
    }

    return 0;
}

tuna_error
tuna_get_index(tuna_device const *device, int *index) {
    *index = device->index;
    return 0;
}

void
tuna_free_name(char *name) {
    free(name);
}

static
tuna_error
tuna_get_name_byob(tuna_device const *device, char *name) {
    if (device->fd != -1) {
        struct ifreq ifr;
        if (ioctl(device->fd, TUNGETIFF, &ifr) == -1) {
            return tuna_translate_syserr(errno);
        }
        strcpy(name, ifr.ifr_name);
    } else {
        if (!if_indextoname(device->index, name)) {
            return tuna_translate_syserr(errno);
        }
    }
    return 0;
}

tuna_error
tuna_get_name(tuna_device const *device, char **name_out) {
    char *name = NULL;

    int err = 0;

    if (!(name = malloc(IFNAMSIZ))) {
        err = tuna_translate_syserr(errno);
        goto out;
    }

    if ((err = tuna_get_name_byob(device, name))) {
        goto out;
    }

    *name_out = name; name = NULL;

out:
    tuna_free_name(name);

    return err;
}

static
void
tuna_initialize_device(tuna_device *device) {
    *device = (tuna_device){
        .fd = -1,
    };
}

static
tuna_error
tuna_allocate_device(tuna_device **device_out) {
    tuna_device *device;
    if (!(device = malloc(sizeof(*device)))) {
        return tuna_translate_syserr(errno);
    }
    tuna_initialize_device(device);

    *device_out = device;

    return 0;
}

//static
//tuna_error
//tuna_disable_default_local_ip6_addr(tuna_device *device) {
//    struct nl_msg *nl_msg = NULL;
//    
//    int err = 0;
//
//    if (!(nl_msg = nlmsg_alloc_simple(RTM_NEWLINK, 0))) {
//        err = TUNA_OUT_OF_MEMORY;
//        goto out;
//    }
//
//    struct ifinfomsg ifi = {.ifi_index = device->index};
//    if ((err = nlmsg_append(nl_msg, &ifi, sizeof(ifi), NLMSG_ALIGNTO))) {
//        err = tuna_translate_nlerr(-err);
//        goto out;
//    }
//
//    struct nlattr *ifla_af_spec_nlattr;
//    if (!(ifla_af_spec_nlattr = nla_nest_start(nl_msg, IFLA_AF_SPEC))) {
//        err = TUNA_OUT_OF_MEMORY;
//        goto out;
//    }
//
//    struct nlattr *af_inet6_nlattr;
//    if (!(af_inet6_nlattr = nla_nest_start(nl_msg, AF_INET6))) {
//        err = TUNA_OUT_OF_MEMORY;
//        goto out;
//    }
//
//    if ((err = nla_put_u8(nl_msg, IFLA_INET6_ADDR_GEN_MODE,
//                                  IN6_ADDR_GEN_MODE_NONE)))
//    {
//        err = tuna_translate_nlerr(-err);
//        goto out;
//    }
//
//    nla_nest_end(nl_msg, af_inet6_nlattr);
//
//    nla_nest_end(nl_msg, ifla_af_spec_nlattr);
//
//    if ((err = nl_send_auto(device->nl_sock, nl_msg)) < 0) {
//        err = tuna_translate_nlerr(-err);
//        goto out;
//    }
//
//    if ((err = nl_wait_for_ack(device->nl_sock))) {
//        err = tuna_translate_nlerr(-err);
//        goto out;
//    }
//
//out:
//    nlmsg_free(nl_msg);
//
//    return err;
//}

tuna_error
tuna_open_device(tuna_device **device_out, tuna_ownership ownership,
                 tuna_device const *attach_target)
{
    tuna_device *device = NULL;

    int err = 0;

    if ((err = tuna_allocate_device(&device))) {
        goto out;
    }

open:
    if ((device->fd = open("/dev/net/tun", O_RDWR)) == -1) {
        err = tuna_translate_syserr(errno);
        goto out;
    }

    struct ifreq ifr = {
        .ifr_flags = IFF_TUN | IFF_NO_PI,
    };
    if (ownership == TUNA_SHARED) {
        ifr.ifr_flags |= IFF_MULTI_QUEUE;
    }
    if (attach_target) {
        if ((err = tuna_get_name_byob(attach_target, ifr.ifr_name))) {
            goto out;
        }
    }
    if (ioctl(device->fd, TUNSETIFF, &ifr) == -1) {
        if (errno == EINVAL) { // IFF_MULTI_QUEUE flag mismatch
            err = TUNA_DEVICE_BUSY; // TODO: maybe replace with special error
        } else {
            err = tuna_translate_syserr(errno);
        }
        goto out;
    }

    if (attach_target) {
        device->index = attach_target->index;
    } else {
        if (!(device->index = if_nametoindex(ifr.ifr_name))) {
            if (errno == ENODEV) {
                close(device->fd); device->fd = -1;
                goto open;
            }
            err = tuna_translate_syserr(errno);
            goto out;
        }
    }

    //if ((err = tuna_disable_default_local_ip6_addr(device))) { goto out; }

    *device_out = device; device = NULL;

out:
    tuna_close_device(device);

    return err;
}

struct tuna_device_list {
    size_t device_count;
    tuna_device devices[];
};

void
tuna_free_device_list(tuna_device_list *list) {
    if (list) {
        for (size_t i = list->device_count; i--;) {
            tuna_finalize_device(&list->devices[i]);
        }
        free(list);
    }
}

size_t
tuna_get_device_count(tuna_device_list const *list) {
    return list->device_count;
}

tuna_device const *
tuna_get_device_at(tuna_device_list const *list, size_t index) {
    return &list->devices[index];
}

static
int
tuna_is_managed(struct rtnl_link *rtnl_link) {
    char const *type = rtnl_link_get_type(rtnl_link);
    return type && !strcmp(type, "tun");
}

tuna_error
tuna_get_device_list(tuna_device_list **list_out) {
    struct nl_sock *nl_sock = NULL;
    struct nl_cache *nl_cache = NULL;
    tuna_device_list *list = NULL;

    int err = 0;

    if ((err = tuna_open_nl_sock(&nl_sock))) {
        goto out;
    }

    if ((err = rtnl_link_alloc_cache(nl_sock, AF_UNSPEC, &nl_cache))) {
        err = tuna_translate_nlerr(-err);
        goto out;
    }

    size_t count = 0;
    for (struct nl_object *nl_object = nl_cache_get_first(nl_cache);
         nl_object; nl_object = nl_cache_get_next(nl_object))
    {
        struct rtnl_link *rtnl_link = (void *)nl_object;
        count += tuna_is_managed(rtnl_link);
    }

    if (!(list = malloc(sizeof(*list) + count * sizeof(*list->devices)))) {
        err = tuna_translate_syserr(errno);
        goto out;
    }
    list->device_count = 0;

    for (struct nl_object *nl_object = nl_cache_get_first(nl_cache);
         nl_object; nl_object = nl_cache_get_next(nl_object))
    {
        struct rtnl_link *rtnl_link = (void *)nl_object;
        if (!tuna_is_managed(rtnl_link)) {
            continue;
        }

        tuna_device *device = &list->devices[list->device_count++];
        tuna_initialize_device(device);

        device->index = rtnl_link_get_ifindex(rtnl_link);
    }

    *list_out = list; list = NULL;

out:
    tuna_free_device_list(list);
    nl_cache_free(nl_cache);
    nl_socket_free(nl_sock);

    return err;
}




















//tuna_io_handle 
//tuna_get_io_handle(tuna_device const *dev) {
//    return dev->fd;
//}
//
//tuna_error
//tuna_get_status(tuna_device const *dev, tuna_status *status) {
//    struct rtnl_link *rtnl_link = NULL;
//
//    int err = 0;
//    
//    if ((err = rtnl_link_get_kernel(dev->nl_sock,
//                                    dev->ifindex, NULL, &rtnl_link)))
//    {
//        err = tuna_translate_nlerr(-err);
//        goto out;
//    }
//
//    if (rtnl_link_get_flags(rtnl_link) & IFF_UP) {
//        *status = TUNA_UP;
//    } else {
//        *status = TUNA_DOWN;
//    }
//
//  out:;
//    rtnl_link_put(rtnl_link);
//
//    return err;
//}
//
//tuna_error
//tuna_set_status(tuna_device *dev, tuna_status status) {
//    struct rtnl_link *rtnl_link = NULL;
//    struct rtnl_link *rtnl_link_patch = NULL;
//
//    int err = 0;
//
//    if ((err = rtnl_link_get_kernel(dev->nl_sock,
//                                    dev->ifindex, NULL, &rtnl_link)))
//    {
//        err = tuna_translate_nlerr(-err);
//        goto out;
//    }
//
//    if (!(rtnl_link_patch = rtnl_link_alloc())) {
//        err = TUNA_OUT_OF_MEMORY;
//        goto out;
//    }
//
//    switch (status) {
//      case TUNA_DOWN:;
//        rtnl_link_unset_flags(rtnl_link_patch, IFF_UP);
//        break;
//      case TUNA_UP:;
//        rtnl_link_set_flags(rtnl_link_patch, IFF_UP);
//        break;
//      default:;
//        assert(0);
//    }
//
//    if ((err = rtnl_link_change(dev->nl_sock,
//                                rtnl_link, rtnl_link_patch, 0)))
//    {
//        err = tuna_translate_nlerr(-err);
//        goto out;
//    }
//
//  out:;
//    rtnl_link_put(rtnl_link_patch);
//    rtnl_link_put(rtnl_link);
//
//    return err;
//}
//
//tuna_error
//tuna_get_index(tuna_device const *dev, int *index) {
//    *index = dev->ifindex;
//    return 0;
//}
//
//tuna_error
//tuna_get_name(tuna_device const *device, char const **name) {
//    tuna_device *dev = (void *)device;
//
//    struct rtnl_link *rtnl_link = NULL;
//
//    int err = 0;
//    
//    if ((err = rtnl_link_get_kernel(dev->nl_sock,
//                                    dev->ifindex, NULL, &rtnl_link)))
//    {
//        err = tuna_translate_nlerr(-err);
//        goto out;
//    }
//
//    strcpy(dev->name, rtnl_link_get_name(rtnl_link));
//    *name = dev->name;
//
//  out:;
//    rtnl_link_put(rtnl_link);
//
//    return err;
//}
//
//tuna_error
//tuna_set_name(tuna_device *dev, char const *name) {
//    struct rtnl_link *rtnl_link = NULL;
//    struct rtnl_link *rtnl_link_patch = NULL;
//
//    int err = 0;
//
//    size_t name_len = strlen(name);
//    if (name_len < 1) {
//        err = TUNA_INVALID_NAME;
//        goto out;
//    }
//    if (name_len >= IFNAMSIZ) {
//        err = TUNA_NAME_TOO_LONG;
//        goto out;
//    }
//
//    if ((err = rtnl_link_get_kernel(dev->nl_sock,
//                                    dev->ifindex, NULL, &rtnl_link)))
//    {
//        err = tuna_translate_nlerr(-err);
//        goto out;
//    }
//
//    if (!(rtnl_link_patch = rtnl_link_alloc())) {
//        err = TUNA_OUT_OF_MEMORY;
//        goto out;
//    }
//
//    rtnl_link_set_name(rtnl_link_patch, name);
//
//    switch ((err = rtnl_link_change(dev->nl_sock,
//                                    rtnl_link, rtnl_link_patch, 0)))
//    {
//      case 0:;
//        break;
//      case -NLE_EXIST:;
//        err = TUNA_DUPLICATE_NAME;
//        goto out;
//      case -NLE_INVAL:;
//        err = TUNA_INVALID_NAME;
//        goto out;
//      case -NLE_BUSY:;
//        err = TUNA_DEVICE_BUSY;
//        goto out;
//      default:;
//        err = tuna_translate_nlerr(-err);
//        goto out;
//    }
//
//  out:;
//    rtnl_link_put(rtnl_link_patch);
//    rtnl_link_put(rtnl_link);
//
//    return err;
//}
//
//tuna_error
//tuna_get_mtu(tuna_device const *dev, size_t *mtu) {
//    struct rtnl_link *rtnl_link = NULL;
//
//    int err = 0;
//    
//    if ((err = rtnl_link_get_kernel(dev->nl_sock,
//                                    dev->ifindex, NULL, &rtnl_link)))
//    {
//        err = tuna_translate_nlerr(-err);
//        goto out;
//    }
//
//    *mtu = rtnl_link_get_mtu(rtnl_link);
//
//  out:;
//    rtnl_link_put(rtnl_link);
//
//    return err;
//}
//
//tuna_error
//tuna_set_mtu(tuna_device *dev, size_t mtu) {
//    struct rtnl_link *rtnl_link = NULL;
//    struct rtnl_link *rtnl_link_patch = NULL;
//
//    int err = 0;
//
//    if ((err = rtnl_link_get_kernel(dev->nl_sock,
//                                    dev->ifindex, NULL, &rtnl_link)))
//    {
//        err = tuna_translate_nlerr(-err);
//        goto out;
//    }
//
//    if (!(rtnl_link_patch = rtnl_link_alloc())) {
//        err = TUNA_OUT_OF_MEMORY;
//        goto out;
//    }
//
//    rtnl_link_set_mtu(rtnl_link_patch, mtu);
//
//    switch ((err = rtnl_link_change(dev->nl_sock,
//                                    rtnl_link, rtnl_link_patch, 0)))
//    {
//      case 0:;
//        break;
//      case -NLE_INVAL:;
//        size_t cur_mtu = rtnl_link_get_mtu(rtnl_link);
//        if (mtu < cur_mtu) {
//            err = TUNA_MTU_TOO_SMALL;
//        } else if (mtu > cur_mtu) {
//            err = TUNA_MTU_TOO_BIG;
//        } else {
//            err = TUNA_UNEXPECTED;
//        }
//        goto out;
//      default:;
//        err = tuna_translate_nlerr(-err);
//        goto out;
//    }
//
//  out:;
//    rtnl_link_put(rtnl_link_patch);
//    rtnl_link_put(rtnl_link);
//
//    return err;
//}
//
//static
//int
//tuna_addr_from_nl(tuna_address *addr, struct nl_addr *nl_addr) {
//    switch (nl_addr_get_family(nl_addr)) {
//      case AF_INET:;
//        addr->family = TUNA_IP4;
//        memcpy(addr->ip4.value, nl_addr_get_binary_addr(nl_addr), 4);
//        addr->ip4.prefix_length = nl_addr_get_prefixlen(nl_addr);
//        return 1;
//      case AF_INET6:;
//        addr->family = TUNA_IP6;
//        memcpy(addr->ip6.value, nl_addr_get_binary_addr(nl_addr), 16);
//        addr->ip6.prefix_length = nl_addr_get_prefixlen(nl_addr);
//        return 1;
//      default:;
//        return 0;
//    }
//}
//
//tuna_error
//tuna_get_addresses(tuna_device const *device,
//                   tuna_address const **addresses, size_t *count)
//{
//    tuna_device *dev = (void *)device;
//
//    struct nl_cache *nl_cache = NULL;
//
//    int err = 0;
//
//    if ((err = rtnl_addr_alloc_cache(dev->nl_sock, &nl_cache))) {
//        err = tuna_translate_nlerr(-err);
//        goto out;
//    }
//
//    size_t cnt = 0;
//    for (struct nl_object *nl_object = nl_cache_get_first(nl_cache);
//         nl_object; nl_object = nl_cache_get_next(nl_object))
//    {
//        if (cnt == dev->addrs_cap) {
//            size_t addrs_cap = dev->addrs_cap * 5 / 3 + 1;
//            tuna_address *addrs = realloc(dev->addrs,
//                                          addrs_cap * sizeof(*addrs));
//            if (!addrs) {
//                err = TUNA_OUT_OF_MEMORY;
//                goto out;
//            }
//            dev->addrs = addrs;
//            dev->addrs_cap = addrs_cap;
//        }
//        struct rtnl_addr *rtnl_addr = (void *)nl_object;
//        
//        if (rtnl_addr_get_ifindex(rtnl_addr) != dev->ifindex) { continue; }
//
//        struct nl_addr *nl_addr = rtnl_addr_get_local(rtnl_addr);
//        if (!tuna_addr_from_nl(dev->addrs + cnt, nl_addr)) { continue; }
//
//        ++cnt;
//    }
//
//    tuna_address *addrs = realloc(dev->addrs, cnt * sizeof(*addrs));
//    if (!cnt || addrs) {
//        dev->addrs = addrs;
//        dev->addrs_cap = cnt;
//    }
//
//    *addresses = dev->addrs;
//    *count = cnt;
//
//  out:;
//    nl_cache_free(nl_cache);
//
//    return err;
//}
//
//static
//tuna_error
//tuna_addr_to_nl(tuna_address const *addr, struct nl_addr **nl_address) {
//    struct nl_addr *nl_addr = NULL;
//
//    int err = 0;
//
//    switch (addr->family) {
//      case TUNA_IP4:;
//        if (!(nl_addr = nl_addr_build(AF_INET, addr->ip4.value, 4))) {
//            err = TUNA_OUT_OF_MEMORY;
//            goto out;
//        }
//        assert(addr->ip4.prefix_length <= 32);
//        nl_addr_set_prefixlen(nl_addr, addr->ip4.prefix_length);
//        break;
//      case TUNA_IP6:;
//        if (!(nl_addr = nl_addr_build(AF_INET6, addr->ip6.value, 16))) {
//            err = TUNA_OUT_OF_MEMORY;
//            goto out;
//        }
//        assert(addr->ip6.prefix_length <= 128);
//        nl_addr_set_prefixlen(nl_addr, addr->ip6.prefix_length);
//        break;
//      default:;
//        assert(0);
//    }
//
//    *nl_address = nl_addr;
//
//  out:;
//    if (err) { nl_addr_put(nl_addr); }
//
//    return err;
//}
//
//tuna_error
//tuna_add_address(tuna_device *dev, tuna_address const *addr) {
//    struct nl_addr *nl_addr = NULL;
//    struct rtnl_addr *rtnl_addr = NULL;
//
//    int err = 0;
//
//    if ((err = tuna_addr_to_nl(addr, &nl_addr))) { goto out; }
//
//    if (!(rtnl_addr = rtnl_addr_alloc())) {
//        err = TUNA_OUT_OF_MEMORY;
//        goto out;
//    }
//
//    rtnl_addr_set_ifindex(rtnl_addr, dev->ifindex);
//    if ((err = rtnl_addr_set_local(rtnl_addr, nl_addr))) {
//        err = tuna_translate_nlerr(-err);
//        goto out;
//    }
//
//    if ((err = rtnl_addr_add(dev->nl_sock, rtnl_addr, NLM_F_REPLACE))) {
//        err = tuna_translate_nlerr(-err);
//        goto out;
//    }
//
//  out:;
//    rtnl_addr_put(rtnl_addr);
//    nl_addr_put(nl_addr);
//
//    return err;
//}
//
//tuna_error
//tuna_remove_address(tuna_device *dev, tuna_address const *addr) {
//    struct nl_addr *nl_addr = NULL;
//    struct rtnl_addr *rtnl_addr = NULL;
//
//    int err = 0;
//
//    if ((err = tuna_addr_to_nl(addr, &nl_addr))) { goto out; }
//
//    if (!(rtnl_addr = rtnl_addr_alloc())) {
//        err = TUNA_OUT_OF_MEMORY;
//        goto out;
//    }
//
//    rtnl_addr_set_ifindex(rtnl_addr, dev->ifindex);
//    if ((err = rtnl_addr_set_local(rtnl_addr, nl_addr))) {
//        err = tuna_translate_nlerr(-err);
//        goto out;
//    }
//
//    switch ((err = rtnl_addr_delete(dev->nl_sock, rtnl_addr, 0))) {
//      case 0:;
//        break;
//      case -NLE_NOADDR:;
//        err = 0;
//        break;
//      default:;
//        err = tuna_translate_nlerr(-err);
//        goto out;
//    }
//
//  out:;
//    rtnl_addr_put(rtnl_addr);
//    nl_addr_put(nl_addr);
//
//    return err;
//}
