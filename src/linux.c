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
            return TUNA_NOT_ENOUGH_MEMORY;
          case EMFILE:;
            return TUNA_TOO_MANY_HANDLES_OPEN;
          case ENFILE:;
            return TUNA_TOO_MANY_HANDLES_OPEN_IN_SYSTEM;
        }
        return TUNA_UNEXPECTED;
    }

  create_tunnel:;
    int fd = open("/dev/net/tun", O_RDWR);
    if (fd == -1) {
        TUNA_FREEZE_ERRNO { close(rtnl_sockfd); }
        switch (errno) {
          case ENOMEM:;
            return TUNA_NOT_ENOUGH_MEMORY;
          case EMFILE:;
            return TUNA_TOO_MANY_HANDLES_OPEN;
          case ENFILE:;
            return TUNA_TOO_MANY_HANDLES_OPEN_IN_SYSTEM;
        }
        return TUNA_UNEXPECTED;
    }

    struct ifreq ifr = {.ifr_flags = IFF_TUN | IFF_NO_PI};
    if (ioctl(fd, TUNSETIFF, &ifr) == -1) {
        TUNA_FREEZE_ERRNO {
            close(fd);
            close(rtnl_sockfd);
        }
        switch (errno) {
          case EPERM:;
            return TUNA_OPERATION_NOT_PERMITTED;
        }
        return TUNA_UNEXPECTED;
    }

    if (ioctl(rtnl_sockfd, SIOCGIFINDEX, &ifr) == -1) {
        TUNA_FREEZE_ERRNO {
            close(fd);
            close(rtnl_sockfd);
        }
        switch (errno) {
          case ENODEV:;
            goto create_tunnel;
        }
        return TUNA_UNEXPECTED;
    }

    device->priv_fd = fd;
    device->priv_ifindex = ifr.ifr_ifindex;
    device->priv_rtnl_sockfd = rtnl_sockfd;
    return 0;
}

tuna_error_t
tuna_destroy(tuna_device_t *device) {
    if (close(device->priv_fd) == -1) {
        TUNA_FREEZE_ERRNO { close(device->priv_rtnl_sockfd); }
        return TUNA_UNEXPECTED;
    }
    if (close(device->priv_rtnl_sockfd) == -1) {
        return TUNA_UNEXPECTED;
    }
    return 0;
}


tuna_error_t
tuna_get_name(tuna_device_t const *device, char *name, size_t *length) {
    struct ifreq ifr = {.ifr_ifindex = device->priv_ifindex};
    if (ioctl(device->priv_rtnl_sockfd, SIOCGIFNAME, &ifr) == -1) {
        return TUNA_UNEXPECTED;
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
tuna_get_status(tuna_device_t const *device, tuna_status_t *status) {
  start:;
    struct ifreq ifr = {.ifr_ifindex = device->priv_ifindex};
    if (ioctl(device->priv_rtnl_sockfd, SIOCGIFNAME, &ifr) == -1) {
        return TUNA_UNEXPECTED;
    }
    if (ioctl(device->priv_rtnl_sockfd, SIOCGIFFLAGS, &ifr) == -1) {
        switch (errno) {
          case ENODEV:;
            goto start;
        }
        return TUNA_UNEXPECTED;
    }
    *status = (ifr.ifr_flags & IFF_UP) ? TUNA_CONNECTED : TUNA_DISCONNECTED;
    return 0;
}

//tuna_error_t
//tuna_set_status(tuna_device_t const *device, tuna_status_t status) {
//    if (ioctl(rtnl_sockfd, SIOCGIFFLAGS, &ifr) == -1) {
//        TUNA_FREEZE_ERRNO {
//            close(rtnl_sockfd);
//            close(fd);
//        }
//        return TUNA_UNEXPECTED;
//    }
//    ifr.ifr_flags |= IFF_UP;
//    ifr.ifr_flags &= ~IFF_LOWER_UP;
//    if (ioctl(rtnl_sockfd, SIOCSIFFLAGS, &ifr) == -1) {
//        TUNA_FREEZE_ERRNO {
//            close(rtnl_sockfd);
//            close(fd);
//        }
//        switch (errno) {
//          case EPERM:
//            return TUNA_OPERATION_NOT_PERMITTED;
//        }
//        return TUNA_UNEXPECTED;
//    }
//}

///////////////////////////////////////////////////////////////////////////////
#endif
