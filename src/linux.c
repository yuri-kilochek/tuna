#include <tuna.h>

#include <errno.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if.h>
#include <linux/if_tun.h>

#include <netlink/socket.h>
#include <netlink/route/link.h>
#include <netlink/route/addr.h>

///////////////////////////////////////////////////////////////////////////////

struct tuna_device {
    int index;
    int fd;
};

static
void
tuna_deinitialize_device(tuna_device *device) {
    if (device->fd != -1) {
        close(device->fd);
    }
}

void
tuna_free_device(tuna_device *device) {
    if (device) {
        tuna_deinitialize_device(device);
        free(device);
    }
}

static
tuna_error
tuna_translate_nl_error(int err) {
    switch (err) {
    case 0:
        return 0;
    default:
        return TUNA_UNEXPECTED;
    case NLE_NODEV:
    case NLE_OBJ_NOTFOUND:
        return TUNA_DEVICE_LOST;
    case NLE_PERM:
    case NLE_NOACCESS:
        return TUNA_FORBIDDEN;
    case NLE_NOMEM:
        return TUNA_OUT_OF_MEMORY;
    case NLE_BUSY:
        return TUNA_DEVICE_BUSY;
    }
}

static
tuna_error
tuna_get_raw_rtnl_link_via(struct nl_sock *nl_sock, int index,
                           nl_recvmsg_msg_cb_t callback, void *context)
{
    struct nl_cb *nl_cb = NULL;
    struct nl_msg *nl_msg = NULL;

    int err = 0;

    if (!(nl_cb = nl_cb_alloc(NL_CB_CUSTOM))) {
        err = TUNA_OUT_OF_MEMORY;
        goto out;
    }

    if ((err = nl_cb_set(nl_cb, NL_CB_VALID, NL_CB_CUSTOM, callback, context))
     || (err = rtnl_link_build_get_request(index, NULL, &nl_msg))
     || (err = nl_send_auto(nl_sock, nl_msg)) < 0
     || (err = nl_recvmsgs(nl_sock, nl_cb)))
    {
        err = tuna_translate_nl_error(-err);
        goto out;
    }

out:
    nlmsg_free(nl_msg);
    nl_cb_put(nl_cb);

    return err;
}

static
tuna_error
tuna_open_nl_sock(struct nl_sock **nl_sock_out) {
    struct nl_sock *nl_sock = NULL;

    int err = 0;

    if (!(nl_sock = nl_socket_alloc())) {
        err = TUNA_OUT_OF_MEMORY;
        goto out;
    }

    if ((err = nl_connect(nl_sock, NETLINK_ROUTE))) {
        err = tuna_translate_nl_error(-err);
        goto out;
    }

    *nl_sock_out = nl_sock; nl_sock = NULL;

out:
    nl_socket_free(nl_sock);

    return err;
}

static
tuna_error
tuna_get_raw_rtnl_link(int index,
                       nl_recvmsg_msg_cb_t callback, void *context)
{
    struct nl_sock *nl_sock = NULL;

    int err = 0;

    if ((err = tuna_open_nl_sock(&nl_sock)) ||
        (err = tuna_get_raw_rtnl_link_via(nl_sock, index, callback, context)))
    { goto out; }

out:
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
    tuna_ownership *ownership_out = context;

    struct nlattr *nlattr = tuna_find_ifinfo_nlattr(nl_msg, IFLA_LINKINFO);
    nlattr = tuna_nested_find_nlattr(nlattr, IFLA_INFO_DATA);
    nlattr = tuna_nested_find_nlattr(nlattr, IFLA_TUN_MULTI_QUEUE);
    *ownership_out = nla_get_u8(nlattr);

    return NL_STOP;
}

tuna_error
tuna_get_ownership(tuna_device const *device, tuna_ownership *ownership_out) {
    return tuna_get_raw_rtnl_link(device->index,
                                  tuna_get_ownership_callback, ownership_out);
}

static
int
tuna_get_lifetime_callback(struct nl_msg *nl_msg, void *context) {
    tuna_lifetime *lifetime_out = context;

    struct nlattr *nlattr = tuna_find_ifinfo_nlattr(nl_msg, IFLA_LINKINFO);
    nlattr = tuna_nested_find_nlattr(nlattr, IFLA_INFO_DATA);
    nlattr = tuna_nested_find_nlattr(nlattr, IFLA_TUN_PERSIST);
    *lifetime_out = nla_get_u8(nlattr);

    return NL_STOP;
}

tuna_error
tuna_get_lifetime(tuna_device const *device, tuna_lifetime *lifetime_out) {
    return tuna_get_raw_rtnl_link(device->index,
                                  tuna_get_lifetime_callback, lifetime_out);
}

static
tuna_error
tuna_translate_sys_error(int errnum) {
    switch (errnum) {
    case 0:
        return 0;
    default:
        return TUNA_UNEXPECTED;
    case ENXIO:
    case ENODEV:
        return TUNA_DEVICE_LOST;
    case EPERM:
    case EACCES:
        return TUNA_FORBIDDEN;
    case ENOMEM:
    case ENOBUFS:
        return TUNA_OUT_OF_MEMORY;
    case EMFILE:
    case ENFILE:
        return TUNA_TOO_MANY_HANDLES;
    case EBUSY:
        return TUNA_DEVICE_BUSY;
    }
}

tuna_error
tuna_set_lifetime(tuna_device *device, tuna_lifetime lifetime) {
    if (ioctl(device->fd, TUNSETPERSIST, (unsigned long)lifetime) == -1) {
        return tuna_translate_sys_error(errno);
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

    if (!(name = malloc(IF_NAMESIZE))
     || !if_indextoname(device->index, name))
    {
        err = tuna_translate_sys_error(errno);
        goto out;
    }

    *name_out = name; name = NULL;

out:
    tuna_free_name(name);

    return err;
}

static
tuna_error
tuna_get_rtnl_link_via(struct nl_sock *nl_sock, int index,
                       struct rtnl_link **rtnl_link_out)
{
    int err = rtnl_link_get_kernel(nl_sock, index, NULL, rtnl_link_out);
    if (err) { return tuna_translate_nl_error(-err); }
    return 0;
}

static
tuna_error
tuna_allocate_rtnl_link(struct rtnl_link **rtnl_link_out) {
    struct rtnl_link *rtnl_link = rtnl_link_alloc();
    if (!rtnl_link) { return TUNA_OUT_OF_MEMORY; }
    *rtnl_link_out = rtnl_link;
    return 0;
}

static
tuna_error
tuna_change_rtnl_link_via(struct nl_sock *nl_sock,
                          struct rtnl_link *rtnl_link,
                          struct rtnl_link *rtnl_link_changes)
{
    int err = rtnl_link_change(nl_sock, rtnl_link, rtnl_link_changes, 0);
    if (err) { return tuna_translate_nl_error(-err); }
    return 0;
}

static
tuna_error
tuna_change_rtnl_link_flags_via(struct nl_sock *nl_sock, int index,
                                void (*change)(struct rtnl_link *, unsigned),
                                unsigned flags, unsigned *old_flags_out)
{
    struct rtnl_link *rtnl_link = NULL;
    struct rtnl_link *rtnl_link_changes = NULL;

    int err = 0;

    if ((err = tuna_get_rtnl_link_via(nl_sock, index, &rtnl_link))
     || (err = tuna_allocate_rtnl_link(&rtnl_link_changes))
     || (change(rtnl_link_changes, flags), 0)
     || (err = tuna_change_rtnl_link_via(nl_sock,
                                         rtnl_link, rtnl_link_changes)))
    { goto out; }

    if (old_flags_out) {
        *old_flags_out = rtnl_link_get_flags(rtnl_link);
    }

out:
    rtnl_link_put(rtnl_link_changes);
    rtnl_link_put(rtnl_link);

    return err;
}

tuna_error
tuna_set_name(tuna_device *device, char const *name) {
    struct nl_sock *nl_sock = NULL;
    struct rtnl_link *rtnl_link = NULL;
    struct rtnl_link *rtnl_link_changes = NULL;
    unsigned flags = 0;

    int err = 0;

    size_t length = strlen(name);
    if (((length == 0) && (err = TUNA_INVALID_NAME))
     || ((length >= IF_NAMESIZE) && (err = TUNA_NAME_TOO_LONG))
     || (err = tuna_open_nl_sock(&nl_sock))
     || (err = tuna_change_rtnl_link_flags_via(nl_sock, device->index,
                                               rtnl_link_unset_flags,
                                               IFF_UP, &flags))
     || (err = tuna_get_rtnl_link_via(nl_sock, device->index, &rtnl_link))
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

    if (tuna_change_rtnl_link_flags_via(nl_sock, device->index,
                                        rtnl_link_set_flags,
                                        flags & IFF_UP, NULL))
    {
        close(device->fd); device->fd = -1;
        err = TUNA_DEVICE_LOST;
    }

    nl_socket_free(nl_sock);

    return err;
}

static
tuna_error
tuna_get_rtnl_link(int index, struct rtnl_link **rtnl_link_out) {
    struct nl_sock *nl_sock = NULL;
    struct rtnl_link *rtnl_link = NULL;

    int err = 0;

    if ((err = tuna_open_nl_sock(&nl_sock))
     || (err = tuna_get_rtnl_link_via(nl_sock, index, &rtnl_link)))
    { goto out; }

    *rtnl_link_out = rtnl_link; rtnl_link = NULL;

out:
    rtnl_link_put(rtnl_link);
    nl_socket_free(nl_sock);

    return err;
}

tuna_error
tuna_get_mtu(tuna_device const *device, size_t *mtu_out) {
    struct rtnl_link *rtnl_link = NULL;

    int err = 0;

    if ((err = tuna_get_rtnl_link(device->index, &rtnl_link))) {
        goto out;
    }

    *mtu_out = rtnl_link_get_mtu(rtnl_link);

out:
    rtnl_link_put(rtnl_link);

    return err;
}

tuna_error
tuna_set_mtu(tuna_device *device, size_t mtu) {
    struct nl_sock *nl_sock = NULL;
    struct rtnl_link *rtnl_link = NULL;
    struct rtnl_link *rtnl_link_changes = NULL;

    int err = 0;

    if ((err = tuna_open_nl_sock(&nl_sock))
     || (err = tuna_get_rtnl_link_via(nl_sock, device->index, &rtnl_link))
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

tuna_error
tuna_get_status(tuna_device const *device, tuna_status *status_out) {
    struct rtnl_link *rtnl_link = NULL;

    int err = 0;

    if ((err = tuna_get_rtnl_link(device->index, &rtnl_link))) {
        goto out;
    }

    switch (rtnl_link_get_operstate(rtnl_link)) {
    case IF_OPER_UNKNOWN:
    case IF_OPER_UP:
        *status_out = TUNA_CONNECTED;
        break;
    default:
        *status_out = TUNA_DISCONNECTED;
    }

out:
    rtnl_link_put(rtnl_link);

    return err;
}

tuna_error
tuna_set_status(tuna_device *device, tuna_status status) {
    if (ioctl(device->fd, TUNSETCARRIER, &(unsigned int){status}) == -1) {
        return tuna_translate_sys_error(errno);
    }
    return 0;
}

tuna_io_handle 
tuna_get_io_handle(tuna_device *device) {
    return device->fd;
}

static
tuna_error
tuna_address_to_local_nl_addr(int index,
                              tuna_address const *address,
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

    switch (address->family) {
    case TUNA_IP4:;
        tuna_ip4_address const *ip4 = (void *)address;
        if (!(nl_addr = nl_addr_build(AF_INET, ip4->value, 4))) {
            err = TUNA_OUT_OF_MEMORY;
            goto out;
        }
        nl_addr_set_prefixlen(nl_addr, ip4->prefix_length);
        break;
    case TUNA_IP6:;
        tuna_ip6_address const *ip6 = (void *)address;
        if (!(nl_addr = nl_addr_build(AF_INET6, ip6->value, 16))) {
            err = TUNA_OUT_OF_MEMORY;
            goto out;
        }
        nl_addr_set_prefixlen(nl_addr, ip6->prefix_length);
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

tuna_error
tuna_add_address(tuna_device *device, tuna_address const *address) {
    struct nl_sock *nl_sock = NULL;
    struct rtnl_addr *rtnl_addr = NULL;

    int err = 0;

    if ((err = tuna_open_nl_sock(&nl_sock))
     || (err = tuna_address_to_local_nl_addr(device->index,
                                             address, &rtnl_addr)))
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

tuna_error
tuna_remove_address(tuna_device *device, tuna_address const *address) {
    struct nl_sock *nl_sock = NULL;
    struct rtnl_addr *rtnl_addr = NULL;

    int err = 0;

    if ((err = tuna_open_nl_sock(&nl_sock))
     || (err = tuna_address_to_local_nl_addr(device->index,
                                             address, &rtnl_addr)))
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

struct tuna_address_list {
    size_t address_count; // TODO: rename to count
    union {
        tuna_address_family family;
        tuna_ip4_address ip4;
        tuna_ip6_address ip6;
    } addresses[]; // TODO: rename to items
};

void
tuna_free_address_list(tuna_address_list *list) {
    free(list);
}

size_t
tuna_get_address_count(tuna_address_list const *list) {
    return list->address_count;
}

tuna_address const *
tuna_get_address_at(tuna_address_list const *list, size_t index) {
    return (void *)&list->addresses[index];
}

static
int
tuna_local_nl_addr_to_address(int index, struct rtnl_addr *rtnl_addr,
                              tuna_address *address)
{
    if (rtnl_addr_get_ifindex(rtnl_addr) == index) {
        struct nl_addr *nl_addr = rtnl_addr_get_local(rtnl_addr);
        switch (nl_addr_get_family(nl_addr)) {
        case AF_INET:
            if (address) {
                tuna_ip4_address *ip4 = (void *)address;
                ip4->family = TUNA_IP4;
                memcpy(ip4->value, nl_addr_get_binary_addr(nl_addr), 4);
                ip4->prefix_length = nl_addr_get_prefixlen(nl_addr);
            }
            return 1;
        case AF_INET6:
            if (address) {
                tuna_ip6_address *ip6 = (void *)address;
                ip6->family = TUNA_IP6;
                memcpy(ip6->value, nl_addr_get_binary_addr(nl_addr), 16);
                ip6->prefix_length = nl_addr_get_prefixlen(nl_addr);
            }
            return 1;
        }
    }
    return 0;
}

tuna_error
tuna_get_address_list(tuna_device const *device,
                      tuna_address_list **list_out)
{
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
    for (struct nl_object *nl_object = nl_cache_get_first(nl_cache);
         nl_object; nl_object = nl_cache_get_next(nl_object))
    {
        struct rtnl_addr *rtnl_addr = (void *)nl_object;
        count += tuna_local_nl_addr_to_address(device->index, rtnl_addr, NULL);
    }

    if (!(list = malloc(sizeof(*list) + count * sizeof(*list->addresses)))) {
        err = tuna_translate_sys_error(errno);
        goto out;
    }
    list->address_count = count;

    size_t i = 0;
    for (struct nl_object *nl_object = nl_cache_get_first(nl_cache);
         nl_object; nl_object = nl_cache_get_next(nl_object))
    {
        struct rtnl_addr *rtnl_addr = (void *)nl_object;
        i += tuna_local_nl_addr_to_address(device->index, rtnl_addr,
                                           (void *)&list->addresses[i]);
    }

    *list_out = list; list = NULL;

out:
    tuna_free_address_list(list);
    nl_cache_free(nl_cache);
    nl_socket_free(nl_sock);

    return err;
}

#define TUNA_DEVICE_INITIALIZER (tuna_device){ \
    .fd = -1, \
}

static
tuna_error
tuna_allocate_device(tuna_device **device_out) {
    tuna_device *device;
    if (!(device = malloc(sizeof(*device)))) {
        return tuna_translate_sys_error(errno);
    }
    *device = TUNA_DEVICE_INITIALIZER;

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
        ifr->ifr_flags |= IFF_MULTI_QUEUE & -nla_get_u8(nlattr);
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
//        err = tuna_translate_nl_error(-err);
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
//        err = tuna_translate_nl_error(-err);
//        goto out;
//    }
//
//    nla_nest_end(nl_msg, af_inet6_nlattr);
//
//    nla_nest_end(nl_msg, ifla_af_spec_nlattr);
//
//    if ((err = nl_send_auto(device->nl_sock, nl_msg)) < 0) {
//        err = tuna_translate_nl_error(-err);
//        goto out;
//    }
//
//    if ((err = nl_wait_for_ack(device->nl_sock))) {
//        err = tuna_translate_nl_error(-err);
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
tuna_open_device(tuna_device const *attach_target, tuna_ownership ownership,
                 tuna_device **device_out)
{
    struct nl_sock *nl_sock = NULL;
    tuna_device *device = NULL;

    int err = 0;

    if ((err = tuna_open_nl_sock(&nl_sock))
     || (err = tuna_allocate_device(&device)))
    { goto out; }

    if ((device->fd = open("/dev/net/tun", O_RDWR)) == -1) {
        err = tuna_translate_sys_error(errno);
        goto out;
    }

    struct ifreq ifr = {
        .ifr_flags = IFF_TUN | IFF_NO_PI,
    };
    if (attach_target) {
        if ((err = tuna_get_raw_rtnl_link_via(nl_sock, attach_target->index,
                                              tuna_open_device_callback,
                                              &ifr)))
        { goto out; }
    } else {
        ifr.ifr_flags |= IFF_MULTI_QUEUE & -ownership;
    }
    if (ioctl(device->fd, TUNSETIFF, &ifr) == -1) {
        err = tuna_translate_sys_error(errno);
        goto out;
    }

    // XXX: Since we only get the interface's name that can be changed by any
    // priviledged process and there appears to be no TUNGETIFINDEX or other
    // machanism to obtain interface index directly from file destriptor,
    // here we have a potential race condition.
get_index:
    if (ioctl(nl_socket_get_fd(nl_sock), SIOCGIFINDEX, &ifr) == -1) {
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
        if (errno == ENODEV && ioctl(device->fd, TUNGETIFF, &ifr) != -1) {
            goto get_index;
        }
        err = tuna_translate_sys_error(errno);
        goto out;
    }
    device->index = ifr.ifr_ifindex;

    // Even assuming the interface's index has been determined correctly, if
    // the attach target interface has been deleted or renamed just before
    // TUNSETIFF, then TUNSETIFF will succeed and create a new interface
    // instead of attaching to an existing one. At least we can detect this
    // afterwards, since the new interface will have a different index.
    if (attach_target && device->index != attach_target->index) {
        err = TUNA_DEVICE_LOST;
        goto out;
    }

    if ((err = tuna_change_rtnl_link_flags_via(nl_sock, device->index,
                                               rtnl_link_set_flags,
                                               IFF_UP, NULL)))
    { goto out; }

    //if ((err = tuna_disable_default_local_ip6_addr(device))) { goto out; }

    *device_out = device; device = NULL;

out:
    tuna_free_device(device);
    nl_socket_free(nl_sock);

    return err;
}

tuna_error
tuna_create_device(tuna_ownership ownership, tuna_device **device_out) {
    return tuna_open_device(NULL, ownership, device_out);
}

tuna_error
tuna_attach_device(tuna_device const *target, tuna_device **device_out) {
    return tuna_open_device(target, /*ignored*/0, device_out);
}

struct tuna_device_list {
    size_t device_count; // TODO: rename to count
    tuna_device devices[]; // TODO: rename to items
};

void
tuna_free_device_list(tuna_device_list *list) {
    if (list) {
        for (size_t i = 0; i < list->device_count; ++i) {
            tuna_deinitialize_device(&list->devices[i]);
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

// TODO: rename to tuna_load_managed_indices
static
int
tuna_managed_rtnl_link_to_device(struct rtnl_link *rtnl_link,
                                 tuna_device *device)
{
    char const *type = rtnl_link_get_type(rtnl_link);
    if (type && !strcmp(type, "tun")) {
        if (device) {
            device->index = rtnl_link_get_ifindex(rtnl_link);
        }
        return 1;
    }
    return 0;
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
        err = tuna_translate_nl_error(-err);
        goto out;
    }

    size_t count = 0;
    // TODO: move loop into tuna_managed_rtnl_link_to_device
    for (struct nl_object *nl_object = nl_cache_get_first(nl_cache);
         nl_object; nl_object = nl_cache_get_next(nl_object))
    {
        struct rtnl_link *rtnl_link = (void *)nl_object;
        count += tuna_managed_rtnl_link_to_device(rtnl_link, NULL);
    }

    // TODO: move into new tuna_allocate_device_list function
    if (!(list = malloc(sizeof(*list) + count * sizeof(*list->devices)))) {
        err = tuna_translate_sys_error(errno);
        goto out;
    }
    list->device_count = count;
    for (size_t i = 0; i < count; ++i) {
        list->devices[i] = TUNA_DEVICE_INITIALIZER;
    }

    // TODO: move loop into tuna_managed_rtnl_link_to_device
    size_t i = 0;
    for (struct nl_object *nl_object = nl_cache_get_first(nl_cache);
         nl_object; nl_object = nl_cache_get_next(nl_object))
    {
        struct rtnl_link *rtnl_link = (void *)nl_object;
        i += tuna_managed_rtnl_link_to_device(rtnl_link, &list->devices[i]);
    }

    *list_out = list; list = NULL;

out:
    tuna_free_device_list(list);
    nl_cache_free(nl_cache);
    nl_socket_free(nl_sock);

    return err;
}

