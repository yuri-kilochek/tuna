#include <tuna.h>

#if TUNA_PRIV_OS_LINUX
///////////////////////////////////////////////////////////////////////////////

#include <time.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <netinet/in.h>
#include <netlink/netlink.h>

// TODO: delete
#include <stdio.h>

///////////////////////////////////////////////////////////////////////////////

/*
#define TUNA_PRIV_TRY(...) \
    for (tuna_error_t tuna_priv_try_error_##__LINE__ = (__VA_ARGS__); \
         tuna_priv_try_error_##__LINE__; ) \
    for (int tuna_priv_try_state_##__LINE__ = 0; 1; \
         tuna_priv_try_state_##__LINE__ = 1) \
    if (tuna_priv_try_state_##__LINE__) { \
        return tuna_priv_try_error_##__LINE__; \
    } else

#define TUNA_PRIV_FREEZE_ERRNO \
    for (int tuna_priv_frozen_errno_##__LINE__ = errno, \
             tuna_priv_freeze_errno_state_##__LINE__ = 1; \
         tuna_priv_freeze_errno_state_##__LINE__; \
         (errno = tuna_priv_frozen_errno_##__LINE__), \
         tuna_priv_freeze_errno_state_##__LINE__ = 0)
*/

static
tuna_error_t
tuna_priv_xlate_err(int num) {
    switch (num) {
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
tuna_priv_xlate_nlerr(int num) {
    switch (num) {
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

tuna_error_t
tuna_create(tuna_device_t *device) {
    assert(device);

    struct nl_sock *nl_sock = nl_socket_alloc();
    if (!nl_sock) { return tuna_priv_xlate_err(errno); }

    int nlerrnum = nl_connect(nl_sock, NETLINK_ROUTE);
    if (nlerrnum) {
        nl_socket_free(nl_sock);
        return tuna_priv_xlate_nlerr(-nlerrnum);
    }

  open:;
    int fd = open("/dev/net/tun", O_RDWR);
    if (fd == -1) {
        tuna_error_t error = tuna_priv_xlate_err(errno);

        nl_socket_free(nl_sock);

        return error;
    }

    struct ifreq ifr = {.ifr_flags = IFF_TUN | IFF_NO_PI};
    if (ioctl(fd, TUNSETIFF, &ifr) == -1) {
        tuna_error_t error = tuna_priv_xlate_err(errno);

        close(fd);
        nl_socket_free(nl_sock);

        return error;
    }

    if (ioctl(nl_socket_get_fd(nl_sock), SIOCGIFINDEX, &ifr) == -1) {
        if (errno == ENODEV) {
            close(fd);
            goto open;
        }
        tuna_error_t error = tuna_priv_xlate_err(errno);

        close(fd);
        nl_socket_free(nl_sock);

        return error;
    }

    device->priv_fd = fd;
    device->priv_ifindex = ifr.ifr_ifindex;
    device->priv_nl_sock = nl_sock;

    return 0;
}

void
tuna_destroy(tuna_device_t *device) {
    assert(device);

    nl_socket_free(device->priv_nl_sock);
    close(device->priv_fd);
}


tuna_handle_t
tuna_get_handle(tuna_device_t const *device) {
    assert(device);

    return device->priv_fd;
}


//tuna_error_t
//tuna_get_name(tuna_device_t const *device, char *name, size_t *length) {
//    assert(device);
//    assert(name);
//    assert(length);
//
//    struct ifreq ifr = {.ifr_ifindex = device->priv_ifindex};
//    if (ioctl(device->priv_rtnl_sockfd, SIOCGIFNAME, &ifr) == -1) {
//        switch (errno) {
//          default:;
//            return TUNA_UNEXPECTED;
//        }
//    }
//    size_t raw_length = strnlen(ifr.ifr_name, sizeof(ifr.ifr_name));
//    if (*length > raw_length) { *length = raw_length; }
//    memcpy(name, ifr.ifr_name, *length);
//    name[*length] = '\0';
//    if (*length < raw_length) {
//        *length = raw_length;
//        return TUNA_NAME_TOO_LONG;
//    }
//
//    return 0;
//}
//
//tuna_error_t
//tuna_set_name(tuna_device_t *device, char const *name, size_t *length) {
//    assert(device);
//    assert(name);
//    assert(length);
//
//    if (*length >= IFNAMSIZ) {
//        *length = IFNAMSIZ - 1;
//        return TUNA_NAME_TOO_LONG;
//    }
//
//    struct ifreq ifr = {.ifr_ifindex = device->priv_ifindex};
//    if (ioctl(device->priv_rtnl_sockfd, SIOCGIFNAME, &ifr) == -1) {
//        switch (errno) {
//          default:;
//            return TUNA_UNEXPECTED;
//        }
//    }
//
//    if (!ifr.ifr_name[*length] && !memcmp(name, ifr.ifr_name, *length)) {
//        return 0;
//    }
//
//    if (ioctl(device->priv_rtnl_sockfd, SIOCGIFFLAGS, &ifr) == -1) {
//        switch (errno) {
//          default:;
//            return TUNA_UNEXPECTED;
//        }
//    }
//    short flags = ifr.ifr_flags;
//    if (flags & IFF_UP) {
//        ifr.ifr_flags = flags & ~IFF_UP;
//        if (ioctl(device->priv_rtnl_sockfd, SIOCSIFFLAGS, &ifr) == -1) {
//            switch (errno) {
//              case EPERM:;
//                return TUNA_NOT_PERMITTED;
//              default:;
//                return TUNA_UNEXPECTED;
//            }
//        }
//    }
//
//    memcpy(ifr.ifr_newname, name, *length);
//    ifr.ifr_newname[*length] = '\0';
//    if (ioctl(device->priv_rtnl_sockfd, SIOCSIFNAME, &ifr) == -1) {
//        switch (errno) {
//          case EPERM:;
//            return TUNA_NOT_PERMITTED;
//          case EEXIST:;
//            return TUNA_NAME_IN_USE;
//          default:;
//            return TUNA_UNEXPECTED;
//        }
//    }
//    memcpy(ifr.ifr_name, name, *length);
//    ifr.ifr_name[*length] = '\0';
//
//    if (flags & IFF_UP) {
//        ifr.ifr_flags = flags;
//        if (ioctl(device->priv_rtnl_sockfd, SIOCSIFFLAGS, &ifr) == -1) {
//            switch (errno) {
//              default:;
//                return TUNA_UNEXPECTED;
//            }
//        }
//    }
//
//    return 0;
//}
//
//static
//tuna_error_t
//tuna_priv_get_now(struct timeval *tv) {
//    struct timespec ts;
//    if (clock_gettime(CLOCK_MONOTONIC_RAW, &ts) == -1) {
//        switch (errno) {
//          default:;
//            return TUNA_UNEXPECTED;
//        }
//    }
//    tv->tv_sec = ts.tv_sec;
//    tv->tv_usec = ts.tv_nsec / 1000;
//    return 0;
//}
//
//
//static
//tuna_error_t
//tuna_priv_exchange(tuna_device_t *device,
//                   struct nlmsghdr *req, struct nlmsghdr *res)
//{
//    req->nlmsg_flags |= NLM_F_REQUEST | NLM_F_ACK;
//
//  send:;
//    struct sockaddr_nl snl = {AF_NETLINK};
//    req->nlmsg_seq = device->priv_nlmsg_seq++;
//    ssize_t out_len = sendmsg(device->priv_rtnl_sockfd, &(struct msghdr) {
//        .msg_name = &snl,
//        .msg_namelen = sizeof(snl),
//        .msg_iov = &(struct iovec){req, req->nlmsg_len},
//        .msg_iovlen = 1,
//    }, 0);
//    if (out_len == -1) {
//        switch (errno) {
//          case EINTR:;
//            goto send;
//          case ENOMEM:;
//          case ENOBUFS:;
//            return TUNA_OUT_OF_MEMORY;
//          default:;
//            return TUNA_UNEXPECTED;
//        }
//    }
//    assert(out_len == req->nlmsg_len);
//
//    static struct timeval const max_delay = {0, 500000};
//    struct timeval timeout;
//    TUNA_PRIV_TRY(tuna_priv_get_now(&timeout));
//    timeradd(&timeout, &max_delay, &timeout);
//
//  recv:;
//    struct timeval delay;
//    TUNA_PRIV_TRY(tuna_priv_get_now(&delay));
//    if (timercmp(&delay, &timeout, >=)) { goto send; }
//    timersub(&delay, &timeout, &delay);
//    if (setsockopt(device->priv_rtnl_sockfd, SOL_SOCKET,
//                   SO_RCVTIMEO, &delay, sizeof(delay)) == -1)
//    {
//        switch (errno) {
//          default:;
//            return TUNA_UNEXPECTED;
//        }
//    }
//
//    ssize_t in_len = recvmsg(device->priv_rtnl_sockfd, &(struct msghdr){
//        .msg_name = &snl,
//        .msg_namelen = sizeof(snl),
//        .msg_iov = &(struct iovec){res, res->nlmsg_len},
//        .msg_iovlen = 1,
//    }, 0);
//    if (in_len == -1) {
//        switch (errno) {
//          case EAGAIN:;
//          #if EWOULDBLOCK != EAGAIN
//            case EWOULDBLOCK:;
//          #endif
//            goto send;
//          case EINTR:;
//            goto recv;
//          case ENOMEM:;
//          case ENOBUFS:;
//            return TUNA_OUT_OF_MEMORY;
//          default:;
//            return TUNA_UNEXPECTED;
//        }
//    }
//
//    if (snl.nl_pid != 0) { goto recv; }
//
//    for (struct nlmsghdr *nlmsg = res;
//         NLMSG_OK(nlmsg, in_len);
//         nlmsg = NLMSG_NEXT(nlmsg, in_len))
//    {
//        if (nlmsg->nlmsg_flags & NLM_F_REQUEST) { continue; }
//        assert(!(res->nlmsg_flags & NLM_F_MULTI));
//        if (nlmsg->nlmsg_type == NLMSG_NOOP) { continue; }
//        if (nlmsg->nlmsg_seq != req->nlmsg_seq) { continue; }
//
//        memmove(res, nlmsg, nlmsg->nlmsg_len);
//
//        return 0;
//    }
//    goto recv;
//}
//
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
