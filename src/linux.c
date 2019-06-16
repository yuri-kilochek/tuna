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
tuna_free_device(tuna_device *device) {
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
    case NLE_BUSY:
        return TUNA_DEVICE_BUSY;
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
tuna_find_ifinfo_nlattr(struct nl_msg *nl_msg, int type) {
    return nlmsg_find_attr(nlmsg_hdr(nl_msg), sizeof(struct ifinfomsg), type);
}

static
struct nlattr *
tuna_nested_find_nlattr(struct nlattr *nlattr, int type) {
    return nla_find(nla_data(nlattr), nla_len(nlattr), type);
}

static
int
tuna_get_ownership_callback(struct nl_msg *nl_msg, void *context) {
    tuna_ownership *ownership = context;

    struct nlattr *nlattr = tuna_find_ifinfo_nlattr(nl_msg, IFLA_LINKINFO);
    nlattr = tuna_nested_find_nlattr(nlattr, IFLA_INFO_DATA);
    nlattr = tuna_nested_find_nlattr(nlattr, IFLA_TUN_MULTI_QUEUE);
    *ownership = nla_get_u8(nlattr);

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

    *ownership = !!(ifr.ifr_flags & IFF_MULTI_QUEUE);

    return 0;
}

static
int
tuna_get_lifetime_callback(struct nl_msg *nl_msg, void *context) {
    tuna_lifetime *lifetime = context;

    struct nlattr *nlattr = tuna_find_ifinfo_nlattr(nl_msg, IFLA_LINKINFO);
    nlattr = tuna_nested_find_nlattr(nlattr, IFLA_INFO_DATA);
    nlattr = tuna_nested_find_nlattr(nlattr, IFLA_TUN_PERSIST);
    *lifetime = nla_get_u8(nlattr);

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

    *lifetime = !!(ifr.ifr_flags & IFF_PERSIST);

    return 0;
}

tuna_error
tuna_set_lifetime(tuna_device *device, tuna_lifetime lifetime) {
    if (ioctl(device->fd, TUNSETPERSIST, (unsigned long)lifetime) == -1) {
        return tuna_translate_syserr(errno);
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

tuna_error
tuna_get_name(tuna_device const *device, char **name_out) {
    char *name = NULL;

    int err = 0;

    if (!(name = malloc(IFNAMSIZ))) {
        err = tuna_translate_syserr(errno);
        goto out;
    }

    if (device->fd != -1) {
        struct ifreq ifr;
        if (ioctl(device->fd, TUNGETIFF, &ifr) == -1) {
            err = tuna_translate_syserr(errno);
            goto out;
        }
        strcpy(name, ifr.ifr_name);
    } else {
        if (!if_indextoname(device->index, name)) {
            err = tuna_translate_syserr(errno);
            goto out;
        }
    }

    *name_out = name; name = NULL;

out:
    tuna_free_name(name);

    return err;
}

tuna_error
tuna_set_name(tuna_device *device, char const *name) {
    struct nl_sock *nl_sock = NULL;
    struct rtnl_link *rtnl_link = NULL;
    struct rtnl_link *rtnl_link_patch = NULL;

    int err = 0;

    size_t name_len = strlen(name);
    if (name_len == 0) {
        err = TUNA_INVALID_NAME;
        goto out;
    }
    if (name_len >= IFNAMSIZ) {
        err = TUNA_NAME_TOO_LONG;
        goto out;
    }

    if ((err = tuna_open_nl_sock(&nl_sock))) {
        goto out;
    }

    if ((err = rtnl_link_get_kernel(nl_sock,
                                    device->index, NULL, &rtnl_link)))
    {
        err = tuna_translate_nlerr(-err);
        goto out;
    }

    errno = 0;
    if (!(rtnl_link_patch = rtnl_link_alloc())) {
        if (errno) {
            err = tuna_translate_syserr(errno);
        } else {
            err = TUNA_OUT_OF_MEMORY;
        }
        goto out;
    }

    rtnl_link_set_name(rtnl_link_patch, name);

    if ((err = rtnl_link_change(nl_sock, rtnl_link, rtnl_link_patch, 0))) {
        switch (err) {
        case -NLE_INVAL:
            err = TUNA_INVALID_NAME;
            goto out;
        case -NLE_EXIST:
            err = TUNA_DUPLICATE_NAME;
            goto out;
        default:
            err = tuna_translate_nlerr(-err);
            goto out;
        }
    }

out:
    rtnl_link_put(rtnl_link_patch);
    rtnl_link_put(rtnl_link);
    nl_socket_free(nl_sock);

    return err;
}

tuna_io_handle 
tuna_get_io_handle(tuna_device *device) {
    return device->fd;
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

static
int
tuna_open_device_callback(struct nl_msg *nl_msg, void *context) {
    struct ifreq *ifr = context;

    {
        struct nlattr *nlattr = tuna_find_ifinfo_nlattr(nl_msg, IFLA_IFNAME);
        nla_strlcpy(ifr->ifr_name, nlattr, sizeof(ifr->ifr_name));
    }

    {
        struct nlattr *nlattr = tuna_find_ifinfo_nlattr(nl_msg, IFLA_LINKINFO);
        nlattr = tuna_nested_find_nlattr(nlattr, IFLA_INFO_DATA);
        nlattr = tuna_nested_find_nlattr(nlattr, IFLA_TUN_MULTI_QUEUE);
        ifr->ifr_flags = IFF_MULTI_QUEUE & -nla_get_u8(nlattr);
    }

    return NL_STOP;
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

static
tuna_error
tuna_open_device(tuna_device **device_out, tuna_ownership ownership,
                 tuna_device const *attach_target)
{
    tuna_device *device = NULL;

    int err = 0;

    if ((err = tuna_allocate_device(&device))) {
        goto out;
    }

    if ((device->fd = open("/dev/net/tun", O_RDWR)) == -1) {
        err = tuna_translate_syserr(errno);
        goto out;
    }

    struct ifreq ifr;
    if (attach_target) {
        if (attach_target->fd != -1) {
            if (ioctl(attach_target->fd, TUNGETIFF, &ifr) == -1) {
                err = tuna_translate_syserr(errno);
                goto out;
            }
            ifr.ifr_flags &= IFF_MULTI_QUEUE;
        } else {
            if ((err = tuna_query_nl_link(attach_target->index,
                                          tuna_open_device_callback, &ifr)))
            { goto out; }
        }
        ifr.ifr_flags |= IFF_TUN | IFF_NO_PI;
    } else {
        *ifr.ifr_name = '\0';
        ifr.ifr_flags = IFF_TUN | IFF_NO_PI | (IFF_MULTI_QUEUE & -ownership);
    }
    if (ioctl(device->fd, TUNSETIFF, &ifr) == -1) {
        err = tuna_translate_syserr(errno);
        goto out;
    }

    // XXX: Since we only get the interface's name that can be changed by any
    // priviledged process and there appears to be no TUNGETIFINDEX or other
    // machanism to obtain interface index directly from file destriptor,
    // here we have a potential race condition.
get_index:
    if (!(device->index = if_nametoindex(ifr.ifr_name))) {
        // XXX: The following handles the case when it's just this interface
        // that has been renamed, however if the freed name has then been
        // assigned to another interface, then we'll incorrectly get that
        // interface's index and no error. We can then attempt to refetch the
        // name via TUNGETIFF and check if they match, however it is possible
        // that before we can do that, our interface is renamed back to its
        // original name, causing the check to incorrectly succeed. Since there
        // appears to be no way to completely rule out a race, we content
        // ourselves with only handling the basic case, judging the rest
        // extremely unlikely to ever happen.
        if (errno == ENODEV) {
            if (ioctl(device->fd, TUNGETIFF, &ifr) == -1) {
                err = tuna_translate_syserr(errno);
                goto out;
            }
            goto get_index;
        }
        err = tuna_translate_syserr(errno);
        goto out;
    }

    if (attach_target) {
        // Even assuming the interface's index has been determined correctly,
        // if the attach target interface has been deleted or renamed just
        // before TUNSETIFF, then TUNSETIFF will succeed and create a new
        // interface instead of attaching to an existing one. At least we can
        // detect this afterwards, since the new interface will have a
        // different index.
        if (device->index != attach_target->index) {
            err = TUNA_DEVICE_LOST;
            goto out;
        }
    }

    //if ((err = tuna_disable_default_local_ip6_addr(device))) { goto out; }

    *device_out = device; device = NULL;

out:
    tuna_free_device(device);

    return err;
}

tuna_error
tuna_create_device(tuna_device **device, tuna_ownership ownership) {
    return tuna_open_device(device, ownership, NULL);
}

tuna_error
tuna_attach_device(tuna_device **device, tuna_device const *target) {
    return tuna_open_device(device, (tuna_ownership)0, target);
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
