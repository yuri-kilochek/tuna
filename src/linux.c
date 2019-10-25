#include <tuna.h>

#include <errno.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if.h>
#include <linux/if_tun.h>

///////////////////////////////////////////////////////////////////////////////

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


TUNA_PRIV_API
tuna_error
tuna_open(tuna_ref *ref) {
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

TUNA_PRIV_API
uint_fast8_t
tuna_is_open(tuna_ref const *ref) {
    return ref->fd != -1;
}

TUNA_PRIV_API
void
tuna_close(tuna_ref *ref) {
    if (ref->fd != -1) {
        close(ref->fd); ref->fd = -1;
    }
}


