#include <tuna.h>

#if TUNA_PRIV_OS_LINUX
///////////////////////////////////////////////////////////////////////////////

#include <fcntl.h>
#include <unistd.h>
#include <linux/if.h>
#include <linux/if_tun.h>

TUNA_PRIV_API
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

    struct ifreq ifr = {.ifr_flags = IFF_TUN};
    if (ioctl(fd, TUNSETIFF, &ifr) == -1) {
        close(fd);
        switch (errno) {
          case EPERM:
            return TUNA_OPERATION_NOT_PERMITTED;
        }
        return TUNA_UNEXPECTED;
    }

    device->priv_native_handle = fd;
    return 0;
}

TUNA_PRIV_API
tuna_error_t
tuna_destroy(tuna_device_t *device) {
    if (close(tuna->priv_native_handle) == -1) {
        return TUNA_UNEXPECTED;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
#endif
