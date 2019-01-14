#include <tuna.h>

#if TUNA_PRIV_OS_LINUX
///////////////////////////////////////////////////////////////////////////////

#include <errno.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <netinet/in.h>

// TODO: delete
#include <stdio.h>

///////////////////////////////////////////////////////////////////////////////

//#define TUNA_TRY(...) \
//    for (tuna_error_t tuna_priv_try_error_##__LINE__ = (__VA_ARGS__); \
//         tuna_priv_try_error_##__LINE__; ) \
//    for (int tuna_priv_try_state_##__LINE__ = 0; 1; \
//         tuna_priv_try_state_##__LINE__ = 1) \
//    if (tuna_priv_try_state_##__LINE__) { \
//        return tuna_priv_try_error_##__LINE__; \
//    } else

#define TUNA_FREEZE_ERRNO \
    for (int frozen_errno_##__LINE__ = errno; \
         frozen_errno_##__LINE__; \
         (errno = frozen_errno_##__LINE__), frozen_errno_##__LINE__ = 0)


tuna_error_t
tuna_create(tuna_device_t *device) {
    int rtnl_sockfd = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
    if (rtnl_sockfd == -1) {
        switch (errno) {
          case ENOMEM:;
          case ENOBUFS:;
            return TUNA_OUT_OF_MEMORY;
          case EMFILE:;
          case ENFILE:;
            return TUNA_OUT_OF_HANDLES;
          default:
            return TUNA_UNEXPECTED;
        }
    }

    int fd = open("/dev/net/tun", O_RDWR);
    if (fd == -1) {
        TUNA_FREEZE_ERRNO { close(rtnl_sockfd); }
        switch (errno) {
          case ENOMEM:;
            return TUNA_OUT_OF_MEMORY;
          case EMFILE:;
          case ENFILE:;
            return TUNA_OUT_OF_HANDLES;
          default:
            return TUNA_UNEXPECTED;
        }
    }

    struct ifreq ifr = {.ifr_flags = IFF_TUN | IFF_NO_PI};
    if (ioctl(fd, TUNSETIFF, &ifr) == -1) {
        TUNA_FREEZE_ERRNO {
            close(fd);
            close(rtnl_sockfd);
        }
        switch (errno) {
          case EPERM:;
            return TUNA_PERMISSION_DENIED;
          default:
            return TUNA_UNEXPECTED;
        }
    }

    if (ioctl(rtnl_sockfd, SIOCGIFINDEX, &ifr) == -1) {
        TUNA_FREEZE_ERRNO {
            close(fd);
            close(rtnl_sockfd);
        }
        switch (errno) {
          default:
            return TUNA_UNEXPECTED;
        }
    }

    device->priv_fd = fd;
    device->priv_ifindex = ifr.ifr_ifindex;
    device->priv_rtnl_sockfd = rtnl_sockfd;
    device->priv_nlmsg_seq = 0;
    return 0;
}

tuna_error_t
tuna_destroy(tuna_device_t *device) {
    if (close(device->priv_fd) == -1) {
        TUNA_FREEZE_ERRNO { close(device->priv_rtnl_sockfd); }
        switch (errno) {
          default:
            return TUNA_UNEXPECTED;
        }
    }
    if (close(device->priv_rtnl_sockfd) == -1) {
        switch (errno) {
          default:
            return TUNA_UNEXPECTED;
        }
    }
    return 0;
}


tuna_error_t
tuna_get_name(tuna_device_t const *device, char *name, size_t *length) {
    struct ifreq ifr = {.ifr_ifindex = device->priv_ifindex};
    if (ioctl(device->priv_rtnl_sockfd, SIOCGIFNAME, &ifr) == -1) {
        switch (errno) {
          default:
            return TUNA_UNEXPECTED;
        }
    }
    size_t raw_length = strnlen(ifr.ifr_name, sizeof(ifr.ifr_name));
    if (*length > raw_length) { *length = raw_length; }
    memcpy(name, ifr.ifr_name, *length);
    name[*length] = '\0';
    if (*length < raw_length) {
        *length = raw_length;
        return TUNA_NAME_TOO_LONG;
    }
    return 0;
}

tuna_error_t
tuna_set_name(tuna_device_t *device, char const *name, size_t *length) {
    if (*length >= IFNAMSIZ) {
        *length = IFNAMSIZ - 1;
        return TUNA_NAME_TOO_LONG;
    }

    struct ifreq ifr = {.ifr_ifindex = device->priv_ifindex};
    if (ioctl(device->priv_rtnl_sockfd, SIOCGIFNAME, &ifr) == -1) {
        switch (errno) {
          default:
            return TUNA_UNEXPECTED;
        }
    }

    if (!ifr.ifr_name[*length] && !memcmp(name, ifr.ifr_name, *length)) {
        return 0;
    }

    if (ioctl(device->priv_rtnl_sockfd, SIOCGIFFLAGS, &ifr) == -1) {
        switch (errno) {
          default:
            return TUNA_UNEXPECTED;
        }
    }
    short flags = ifr.ifr_flags;
    if (flags & IFF_UP) {
        ifr.ifr_flags = flags & ~IFF_UP;
        if (ioctl(device->priv_rtnl_sockfd, SIOCSIFFLAGS, &ifr) == -1) {
            switch (errno) {
              case EPERM:;
                return TUNA_PERMISSION_DENIED;
              default:
                return TUNA_UNEXPECTED;
            }
        }
    }

    memcpy(ifr.ifr_newname, name, *length);
    ifr.ifr_newname[*length] = '\0';
    if (ioctl(device->priv_rtnl_sockfd, SIOCSIFNAME, &ifr) == -1) {
        switch (errno) {
          case EPERM:;
            return TUNA_PERMISSION_DENIED;
          case EEXIST:
            return TUNA_NAME_IN_USE;
          default:
            return TUNA_UNEXPECTED;
        }
    }
    memcpy(ifr.ifr_name, name, *length);
    ifr.ifr_name[*length] = '\0';

    if (flags & IFF_UP) {
        ifr.ifr_flags = flags;
        if (ioctl(device->priv_rtnl_sockfd, SIOCSIFFLAGS, &ifr) == -1) {
            switch (errno) {
              default:
                return TUNA_UNEXPECTED;
            }
        }
    }

    return 0;
}

tuna_error_t
tuna_set_ip4_address(tuna_device_t *device,
                     uint_least8_t const octets[4],
                     uint_least8_t prefix_length)
{
    union {
        char buffer[1024];
        struct nlmsghdr header;
    } message = {
        .header = {
            .nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg)),
            .nlmsg_type = RTM_NEWADDR,
            .nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK |
                           NLM_F_CREATE | NLM_F_REPLACE,
            .nlmsg_seq = device->priv_nlmsg_seq++,
        },
    };
    struct nlmsghdr *nlmsg = &message.header;

    struct ifaddrmsg *ifa = NLMSG_DATA(nlmsg);
    *ifa = (struct ifaddrmsg) {
        .ifa_family = AF_INET,
        .ifa_prefixlen = prefix_length,
        .ifa_index = device->priv_ifindex,
    };


    struct rtattr *rta =
        (void *)((char *)nlmsg + NLMSG_ALIGN(nlmsg->nlmsg_len));
    *rta = (struct rtattr) {
        .rta_len = RTA_LENGTH(sizeof(struct in_addr)),
        .rta_type = IFA_LOCAL,
    };

    struct in_addr *ia = RTA_DATA(rta);
    *ia = (struct in_addr) {
        octets[0] << 24 |
        octets[1] << 16 |
        octets[2] <<  8 |
        octets[3]
    };

    nlmsg->nlmsg_len = NLMSG_ALIGN(nlmsg->nlmsg_len) + RTA_ALIGN(rta->rta_len);

    int size = sendto(device->priv_rtnl_sockfd, nlmsg, nlmsg->nlmsg_len, 0,
                      &(struct sockaddr_nl){AF_NETLINK},
                      sizeof(struct sockaddr_nl));
    printf("SENDTO: %d\n", size);

    return 0;
}


tuna_error_t
tuna_get_status(tuna_device_t const *device, tuna_status_t *status) {
    struct ifreq ifr = {.ifr_ifindex = device->priv_ifindex};
    if (ioctl(device->priv_rtnl_sockfd, SIOCGIFNAME, &ifr) == -1) {
        switch (errno) {
          default:
            return TUNA_UNEXPECTED;
        }
    }
    if (ioctl(device->priv_rtnl_sockfd, SIOCGIFFLAGS, &ifr) == -1) {
        switch (errno) {
          default:
            return TUNA_UNEXPECTED;
        }
    }
    *status = (ifr.ifr_flags & IFF_UP) ? TUNA_CONNECTED : TUNA_DISCONNECTED;
    return 0;
}

tuna_error_t
tuna_set_status(tuna_device_t const *device, tuna_status_t status) {
    struct ifreq ifr = {.ifr_ifindex = device->priv_ifindex};
    if (ioctl(device->priv_rtnl_sockfd, SIOCGIFNAME, &ifr) == -1) {
        switch (errno) {
          default:
            return TUNA_UNEXPECTED;
        }
    }
    if (ioctl(device->priv_rtnl_sockfd, SIOCGIFFLAGS, &ifr) == -1) {
        switch (errno) {
          default:
            return TUNA_UNEXPECTED;
        }
    }
    switch (status) {
      case TUNA_DISCONNECTED:
        ifr.ifr_flags &= ~IFF_UP;
        break;
      case TUNA_CONNECTED:
        ifr.ifr_flags |= IFF_UP;
        break;
    }
    if (ioctl(device->priv_rtnl_sockfd, SIOCSIFFLAGS, &ifr) == -1) {
        switch (errno) {
          case EPERM:;
            return TUNA_PERMISSION_DENIED;
          default:
            return TUNA_UNEXPECTED;
        }
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
#endif
