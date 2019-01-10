#include <tuna.h>

#if TUNA_PRIV_OS_LINUX
///////////////////////////////////////////////////////////////////////////////

#include <errno.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <linux/netlink.h>

// TODO: delete
#include <stdio.h>

///////////////////////////////////////////////////////////////////////////////

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

    int ifindex = if_nametoindex(ifr.ifr_name);
    if (ifindex == 0) {
        close(fd);
        switch (errno) {
          case ENOMEM:
          case ENOBUFS:
            return TUNA_NOT_ENOUGH_MEMORY;
          case EMFILE:
            return TUNA_TOO_MANY_HANDLES_OPEN;
          case ENFILE:
            return TUNA_TOO_MANY_HANDLES_OPEN_IN_SYSTEM;
        }
        return TUNA_UNEXPECTED;
    }

    device->priv_fd = fd;
    device->priv_ifindex = ifindex;
    return 0;
}

tuna_error_t
tuna_destroy(tuna_device_t *device) {
    if (close(device->priv_fd) == -1) {
        return TUNA_UNEXPECTED;
    }
    return 0;
}


tuna_error_t
tuna_get_name(tuna_device_t const *device, char *name, size_t *name_length) {
    char tmp[IF_NAMESIZE];
    if (if_indextoname(device->priv_ifindex, tmp) == NULL) {
        switch (errno) {
          case ENOMEM:
          case ENOBUFS:
            return TUNA_NOT_ENOUGH_MEMORY;
          case EMFILE:
            return TUNA_TOO_MANY_HANDLES_OPEN;
          case ENFILE:
            return TUNA_TOO_MANY_HANDLES_OPEN_IN_SYSTEM;
        }
        return TUNA_UNEXPECTED;
    }
    size_t tmp_length = strnlen(tmp, IF_NAMESIZE);

    if (name) {
        size_t n = (tmp_length < *name_length) ? tmp_size : *name_size;
        strncpy(name, tmp, copy_size);
        if (copy_size < *name_size) { name[copy_size] = '\0'; }
        *name_size = copy_size;
        return 0;
    }

}

///////////////////////////////////////////////////////////////////////////////
#endif
