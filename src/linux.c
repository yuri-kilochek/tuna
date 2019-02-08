#include <tuna.h>

#if TUNA_PRIV_OS_LINUX
///////////////////////////////////////////////////////////////////////////////

#include <errno.h>
#include <string.h>
#include <assert.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
//#include <netinet/in.h>
#include <netlink/socket.h>
#include <netlink/route/link.h>

// TODO: delete
#include <stdio.h>

///////////////////////////////////////////////////////////////////////////////

static
tuna_error_t
tuna_priv_xlate_posix_err(int err) {
    switch (err) {
      case 0:
        return 0;
      case ENOMEM:;
      case ENOBUFS:;
        return TUNA_OUT_OF_MEMORY;
      case EMFILE:;
      case ENFILE:;
        return TUNA_OUT_OF_HANDLES;
      case EPERM:;
        return TUNA_NOT_PERMITTED;
      default:;
        return TUNA_UNEXPECTED;
    }
}

static
tuna_error_t
tuna_priv_xlate_libnl_err(int err) {
    switch (err) {
      case 0:
        return 0;
      case NLE_NOMEM:;
        return TUNA_OUT_OF_MEMORY;
      case NLE_PERM:;
        return TUNA_NOT_PERMITTED;
      default:;
        return TUNA_UNEXPECTED;
    }
}

static
tuna_error_t
tuna_priv_get_rtnl_link(tuna_device_t const *dev,
                        struct rtnl_link **rtnl_link)
{
    int err = rtnl_link_get_kernel(dev->priv_nl_sock,
                                   dev->priv_ifindex, NULL, rtnl_link);
    return tuna_priv_xlate_libnl_err(-err);
}

tuna_error_t
tuna_create(tuna_device_t *dev) {
    assert(dev);

    dev->priv_nl_sock = NULL;
    dev->priv_fd = -1;
    struct rtnl_link *rtnl_link = NULL;

    int err;

    if (!(dev->priv_nl_sock = nl_socket_alloc())) {
        err = TUNA_OUT_OF_MEMORY;
        goto fail;
    }

    if ((err = nl_connect(dev->priv_nl_sock, NETLINK_ROUTE))) {
        err = tuna_priv_xlate_libnl_err(-err);
        goto fail;
    }

  open:;
    if ((dev->priv_fd = open("/dev/net/tun", O_RDWR)) == -1) {
        err = tuna_priv_xlate_posix_err(errno);
        goto fail;
    }

    struct ifreq ifreq = {.ifr_flags = IFF_TUN | IFF_NO_PI};
    if (ioctl(dev->priv_fd, TUNSETIFF, &ifreq) == -1) {
        err = tuna_priv_xlate_posix_err(errno);
        goto fail;
    }

    if ((err = rtnl_link_get_kernel(dev->priv_nl_sock,
                                    0, ifreq.ifr_name, &rtnl_link)))
    {
        if (err == -NLE_NODEV) {
            close(dev->priv_fd);
            goto open;
        }
        err = tuna_priv_xlate_libnl_err(-err);
        goto fail;
    }
    dev->priv_ifindex = rtnl_link_get_ifindex(rtnl_link);

    err = 0;
  done:;
    if (rtnl_link) { rtnl_link_put(rtnl_link); }
    return err;

  fail:;
    if (dev->priv_fd != -1) { close(dev->priv_fd); }
    if (dev->priv_nl_sock) { nl_socket_free(dev->priv_nl_sock); }
    goto done;
}

void
tuna_destroy(tuna_device_t *dev) {
    assert(dev);

    close(dev->priv_fd);
    nl_socket_free(dev->priv_nl_sock);
}


tuna_handle_t
tuna_get_handle(tuna_device_t const *dev) {
    assert(dev);

    return dev->priv_fd;
}


tuna_error_t
tuna_get_name(tuna_device_t const *dev, char *name, size_t *len) {
    assert(dev);
    assert(name);
    assert(len);
    assert(*len > 0);

    struct rtnl_link *rtnl_link = NULL;

    int err;

    if ((err = tuna_priv_get_rtnl_link(dev, &rtnl_link))) { goto fail; }

    char *raw_name = rtnl_link_get_name(rtnl_link);
    size_t raw_len = strlen(raw_name);

    if (*len <= raw_len) {
        memcpy(name, raw_name, *len - 1);
        name[*len - 1] = '\0';
        *len = raw_len + 1;
        err = TUNA_NAME_TOO_LONG;
        goto fail;
    }

    memcpy(name, raw_name, raw_len + 1);
    *len = raw_len;

    err = 0;
  done:;
    if (rtnl_link) { rtnl_link_put(rtnl_link); }
    return err;

  fail:
    goto done;
}

tuna_error_t
tuna_set_name(tuna_device_t *dev, char const *name, size_t *len) {
    assert(dev);
    assert(name);
    assert(len);

    if (*len >= IFNAMSIZ) {
        *len = IFNAMSIZ - 1;
        return TUNA_NAME_TOO_LONG;
    }

    struct rtnl_link *rtnl_link = NULL;
    struct rtnl_link *rtnl_link_patch = NULL;

    int err;

    if ((err = tuna_priv_get_rtnl_link(dev, &rtnl_link))) { goto fail; }

    if (!(rtnl_link_patch = rtnl_link_alloc())) {
        err = TUNA_OUT_OF_MEMORY;
        goto fail;
    }
    char namez[IFNAMSIZ];
    memcpy(namez, name, *len);
    namez[*len] = '\0';
    rtnl_link_set_name(rtnl_link_patch, namez);

    if ((err = rtnl_link_change(dev->priv_nl_sock,
                                rtnl_link, rtnl_link_patch, 0)))
    {
        err = tuna_priv_xlate_libnl_err(-err);
        goto fail;
    }

    err = 0;
  done:;
    if (rtnl_link_patch) { rtnl_link_put(rtnl_link_patch); }
    if (rtnl_link) { rtnl_link_put(rtnl_link); }
    return err;

  fail:
    goto done;
}

//tuna_error_t
//tuna_set_ip4_address(tuna_device_t *device,
//                     uint_least8_t const octets[4],
//                     uint_least8_t prefix_length)
//{
//    assert(device);
//    assert(octets);
//    assert(prefix_length <= 32);
//
//    struct {
//        struct nlmsghdr nlmsg;
//        struct ifaddrmsg ifa;
//        struct rtattr rta;
//        union {
//            struct in_addr s;
//            uint8_t octets[4];
//        };
//    } req = {
//        .nlmsg = {
//            .nlmsg_len = (char *)&req.s - (char *)&req.nlmsg + sizeof(req.s),
//            .nlmsg_type = RTM_NEWADDR,
//            .nlmsg_flags = NLM_F_CREATE | NLM_F_REPLACE,
//        },
//        .ifa = {
//            .ifa_family = AF_INET,
//            .ifa_prefixlen = prefix_length,
//            .ifa_index = device->priv_ifindex,
//        },
//        .rta = {
//            .rta_len = (char *)&req.s - (char *)&req.rta + sizeof(req.s),
//            .rta_type = IFA_LOCAL,
//        },
//        .octets = {
//            octets[0],
//            octets[1],
//            octets[2],
//            octets[3],
//        },
//    };
//    assert((char *)&req.ifa - (char *)&req.nlmsg == NLMSG_LENGTH(0));
//    assert((char *)&req.rta - (char *)&req.nlmsg ==
//           NLMSG_SPACE(sizeof(req.ifa)));
//    assert((char *)&req.s - (char *)&req.rta == RTA_LENGTH(0));
//    assert(sizeof(req.s) == sizeof(req.octets));
//
//    struct {
//        struct nlmsghdr nlmsg;
//        struct nlmsgerr err;
//    } res = {
//        .nlmsg.nlmsg_len = (char *)&res.err - (char *)&res.nlmsg +
//                           sizeof(res.err),
//    };
//    assert((char *)&res.err - (char *)&res.nlmsg == NLMSG_LENGTH(0));
//
//    TUNA_PRIV_TRY(tuna_priv_exchange(device, &req.nlmsg, &res.nlmsg));
//    assert(res.nlmsg.nlmsg_type == NLMSG_ERROR);
//    if (res.err.error) {
//        errno = -res.err.error;
//        return TUNA_UNEXPECTED;
//    }
//
//    return 0;
//}
//
//
//tuna_error_t
//tuna_get_status(tuna_device_t const *device, tuna_status_t *status) {
//    assert(device);
//    assert(status);
//
//    struct ifreq ifr = {.ifr_ifindex = device->priv_ifindex};
//    if (ioctl(device->priv_rtnl_sockfd, SIOCGIFNAME, &ifr) == -1) {
//        switch (errno) {
//          default:;
//            return TUNA_UNEXPECTED;
//        }
//    }
//
//    if (ioctl(device->priv_rtnl_sockfd, SIOCGIFFLAGS, &ifr) == -1) {
//        switch (errno) {
//          default:;
//            return TUNA_UNEXPECTED;
//        }
//    }
//    *status = (ifr.ifr_flags & IFF_UP) ? TUNA_CONNECTED : TUNA_DISCONNECTED;
//
//    return 0;
//}
//
//tuna_error_t
//tuna_set_status(tuna_device_t const *device, tuna_status_t status) {
//    assert(device);
//
//    struct ifreq ifr = {.ifr_ifindex = device->priv_ifindex};
//    if (ioctl(device->priv_rtnl_sockfd, SIOCGIFNAME, &ifr) == -1) {
//        switch (errno) {
//          default:;
//            return TUNA_UNEXPECTED;
//        }
//    }
//
//    if (ioctl(device->priv_rtnl_sockfd, SIOCGIFFLAGS, &ifr) == -1) {
//        switch (errno) {
//          default:;
//            return TUNA_UNEXPECTED;
//        }
//    }
//    switch (status) {
//      case TUNA_DISCONNECTED:;
//        ifr.ifr_flags &= ~IFF_UP;
//        break;
//      case TUNA_CONNECTED:;
//        ifr.ifr_flags |= IFF_UP;
//        break;
//    }
//    if (ioctl(device->priv_rtnl_sockfd, SIOCSIFFLAGS, &ifr) == -1) {
//        switch (errno) {
//          case EPERM:;
//            return TUNA_NOT_PERMITTED;
//          default:;
//            return TUNA_UNEXPECTED;
//        }
//    }
//
//    return 0;
//}

///////////////////////////////////////////////////////////////////////////////
#endif
