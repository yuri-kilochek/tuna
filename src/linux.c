#include <tuna.h>

#if TUNA_PRIV_OS_LINUX
///////////////////////////////////////////////////////////////////////////////

#include <errno.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <linux/netlink.h>

tuna_error_t
tuna_create(tuna_device_t *device) {
    int fd = open("/dev/net/tun", O_RDWR);
    if (fd == -1) {
        switch (errno) {
          case ENOMEM:
            return TUNA_NOT_ENOUGH_MEMORY;
          case EMFILE:
            return TUNA_TOO_MANY_HANDLES_OPEN;
          case ENFILE:
            return TUNA_TOO_MANY_HANDLES_OPEN_IN_SYSTEM;
        }
        return TUNA_UNEXPECTED;
    }

    struct ifreq ifr = {.ifr_flags = IFF_TUN | IFF_NO_PI};
    if (ioctl(fd, TUNSETIFF, &ifr) == -1) {
        close(fd);
        switch (errno) {
          case EPERM:
            return TUNA_OPERATION_NOT_PERMITTED;
        }
        return TUNA_UNEXPECTED;
    }

    device->priv_fd = fd;
    return 0;
}

tuna_error_t
tuna_destroy(tuna_device_t *device) {
    if (close(device->priv_fd) == -1) {
        return TUNA_UNEXPECTED;
    }
    return 0;
}

//static
//tuna_error_t
//tuna_priv_open_rtnl(int *fd) {
//    int fg = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
//    if (fd == -1) {
//        return TUNA_UNEXPECTED;
//    }
//    return 0;
//}
//
//TUNA_PRIV_API
//tuna_error_t
//tuna_get_name(tuna_device_t const *device, char *name, size_t *name_length) {
//    tuna_error_t error = tuna_priv_open_netlink_socket();
//}

///////////////////////////////////////////////////////////////////////////////
#endif
