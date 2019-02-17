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

struct tuna_device {
    struct nl_sock *nl_sock;
    int fd; int ifindex;
    char name[IFNAMSIZ];
    tuna_address *addrs; size_t addr_cnt;
};

static
tuna_error
tuna_priv_translate_syserr(int err) {
    switch (err) {
      case 0:;
        return 0;
      case EPERM:;
        return TUNA_FORBIDDEN;
      case ENOMEM:;
      case ENOBUFS:;
        return TUNA_OUT_OF_MEMORY;
      case EMFILE:;
      case ENFILE:;
        return TUNA_TOO_MANY_HANDLES;
      default:;
        return TUNA_UNEXPECTED;
    }
}

static
tuna_error
tuna_priv_translate_nlerr(int err) {
    switch (err) {
      case 0:;
        return 0;
      case NLE_PERM:;
        return TUNA_FORBIDDEN;
      case NLE_NOMEM:;
        return TUNA_OUT_OF_MEMORY;
      case NLE_OBJ_NOTFOUND:;
        return TUNA_DEVICE_LOST;
      default:;
        return TUNA_UNEXPECTED;
    }
}

tuna_error
tuna_create_device(tuna_device **dev) {
    struct rtnl_link *rtnl_link;

    int err = 0;

    tuna_device *d = malloc(sizeof(*d));
    if (!d) {
        err = TUNA_OUT_OF_MEMORY;
        goto fail;
    }
    *d = (tuna_device){.fd = -1};

    if (!(d->nl_sock = nl_socket_alloc())) {
        err = TUNA_OUT_OF_MEMORY;
        goto fail;
    }

    errno = 0;
    switch ((err = nl_connect(d->nl_sock, NETLINK_ROUTE))) {
      case 0:;
        break;
      case -NLE_FAILURE:;
        switch (errno) {
          case EMFILE:;
          case ENFILE:;
            err = TUNA_TOO_MANY_HANDLES;
            goto fail;
        }
      default:;
        err = tuna_priv_translate_nlerr(-err);
        goto fail;
    }

  open:;
    if ((d->fd = open("/dev/net/tun", O_RDWR)) == -1) {
        err = tuna_priv_translate_syserr(errno);
        goto fail;
    }

    struct ifreq ifreq = {.ifr_flags = IFF_TUN | IFF_NO_PI};
    if (ioctl(d->fd, TUNSETIFF, &ifreq) == -1) {
        err = tuna_priv_translate_syserr(errno);
        goto fail;
    }

    if ((err = rtnl_link_get_kernel(d->nl_sock,
                                    0, ifreq.ifr_name, &rtnl_link)))
    {
        if (err == -NLE_OBJ_NOTFOUND) {
            close(d->fd);
            goto open;
        }
        err = tuna_priv_translate_nlerr(-err);
        goto fail;
    }
    d->ifindex = rtnl_link_get_ifindex(rtnl_link);

    *dev = d;
  done:;
    rtnl_link_put(rtnl_link);
    return err;
  fail:;
    tuna_destroy_device(d);
    goto done;
}

TUNA_PRIV_API
void
tuna_destroy_device(tuna_device *dev) {
    if (dev) {
        free(dev->addrs);
        if (dev->fd != -1) { close(dev->fd); }
        nl_socket_free(dev->nl_sock);
    }
    free(dev);
}

tuna_io_handle 
tuna_get_io_handle(tuna_device const *dev) {
    return dev->fd;
}

tuna_error
tuna_get_status(tuna_device const *dev, tuna_status *status) {
    struct rtnl_link *rtnl_link = NULL;

    int err = 0;
    
    if ((err = rtnl_link_get_kernel(dev->nl_sock,
                                    dev->ifindex, NULL, &rtnl_link)))
    {
        err = tuna_priv_translate_nlerr(-err);
        goto fail;
    }

    if (rtnl_link_get_flags(rtnl_link) & IFF_UP) {
        *status = TUNA_ENABLED;
    } else {
        *status = TUNA_DISABLED;
    }

  done:;
    rtnl_link_put(rtnl_link);
    return err;
  fail:;
    goto done;
}

tuna_error
tuna_set_status(tuna_device *dev, tuna_status status) {
    struct rtnl_link *rtnl_link = NULL;
    struct rtnl_link *rtnl_link_patch = NULL;

    int err = 0;

    if ((err = rtnl_link_get_kernel(dev->nl_sock,
                                    dev->ifindex, NULL, &rtnl_link)))
    {
        err = tuna_priv_translate_nlerr(-err);
        goto fail;
    }

    if (!(rtnl_link_patch = rtnl_link_alloc())) {
        err = TUNA_OUT_OF_MEMORY;
        goto fail;
    }

    switch (status) {
      case TUNA_DISABLED:;
        rtnl_link_unset_flags(rtnl_link_patch, IFF_UP);
        break;
      case TUNA_ENABLED:;
        rtnl_link_set_flags(rtnl_link_patch, IFF_UP);
        break;
    }

    if ((err = rtnl_link_change(dev->nl_sock,
                                rtnl_link, rtnl_link_patch, 0)))
    {
        err = tuna_priv_translate_nlerr(-err);
        goto fail;
    }

  done:;
    rtnl_link_put(rtnl_link_patch);
    rtnl_link_put(rtnl_link);
    return err;
  fail:;
    goto done;
}

tuna_error
tuna_get_ifindex(tuna_device const *dev, int *index) {
    *index = dev->ifindex;
    return 0;
}

tuna_error
tuna_get_name(tuna_device const *dev, char const **name) {
    struct rtnl_link *rtnl_link = NULL;

    int err = 0;
    
    if ((err = rtnl_link_get_kernel(dev->nl_sock,
                                    dev->ifindex, NULL, &rtnl_link)))
    {
        err = tuna_priv_translate_nlerr(-err);
        goto fail;
    }

    strcpy(((tuna_device *)dev)->name, rtnl_link_get_name(rtnl_link));
    *name = dev->name;

  done:;
    rtnl_link_put(rtnl_link);
    return err;
  fail:;
    goto done;
}

tuna_error
tuna_set_name(tuna_device *dev, char const *name) {
    struct rtnl_link *rtnl_link = NULL;
    struct rtnl_link *rtnl_link_patch = NULL;

    int err = 0;

    size_t name_len = strlen(name);
    if (name_len < 1) {
        err = TUNA_INVALID_NAME;
        goto fail;
    }
    if (name_len >= IFNAMSIZ) {
        err = TUNA_NAME_TOO_LONG;
        goto fail;
    }

    if ((err = rtnl_link_get_kernel(dev->nl_sock,
                                    dev->ifindex, NULL, &rtnl_link)))
    {
        err = tuna_priv_translate_nlerr(-err);
        goto fail;
    }

    if (!(rtnl_link_patch = rtnl_link_alloc())) {
        err = TUNA_OUT_OF_MEMORY;
        goto fail;
    }

    rtnl_link_set_name(rtnl_link_patch, name);

    switch ((err = rtnl_link_change(dev->nl_sock,
                                    rtnl_link, rtnl_link_patch, 0)))
    {
      case 0:;
        break;
      case -NLE_EXIST:;
        err = TUNA_DUPLICATE_NAME;
        goto fail;
      case -NLE_INVAL:;
        err = TUNA_INVALID_NAME;
        goto fail;
      default:;
        err = tuna_priv_translate_nlerr(-err);
        goto fail;
    }

  done:;
    rtnl_link_put(rtnl_link_patch);
    rtnl_link_put(rtnl_link);
    return err;
  fail:;
    goto done;
}

tuna_error
tuna_get_mtu(tuna_device const *dev, size_t *mtu) {
    struct rtnl_link *rtnl_link = NULL;

    int err = 0;
    
    if ((err = rtnl_link_get_kernel(dev->nl_sock,
                                    dev->ifindex, NULL, &rtnl_link)))
    {
        err = tuna_priv_translate_nlerr(-err);
        goto fail;
    }

    *mtu = rtnl_link_get_mtu(rtnl_link);

  done:;
    rtnl_link_put(rtnl_link);
    return err;
  fail:;
    goto done;
}

tuna_error
tuna_set_mtu(tuna_device *dev, size_t mtu) {
    struct rtnl_link *rtnl_link = NULL;
    struct rtnl_link *rtnl_link_patch = NULL;

    int err = 0;

    if ((err = rtnl_link_get_kernel(dev->nl_sock,
                                    dev->ifindex, NULL, &rtnl_link)))
    {
        err = tuna_priv_translate_nlerr(-err);
        goto fail;
    }

    if (!(rtnl_link_patch = rtnl_link_alloc())) {
        err = TUNA_OUT_OF_MEMORY;
        goto fail;
    }

    rtnl_link_set_mtu(rtnl_link_patch, mtu);

    if ((err = rtnl_link_change(dev->nl_sock,
                                rtnl_link, rtnl_link_patch, 0)))
    {
        size_t cur_mtu = rtnl_link_get_mtu(rtnl_link);
        if (err == -NLE_INVAL) {
            if (mtu < cur_mtu) {
                err = TUNA_MTU_TOO_SMALL;
            } else if (mtu > cur_mtu) {
                err = TUNA_MTU_TOO_BIG;
            } else {
                err = TUNA_UNEXPECTED;
            }
        } else {
            err = tuna_priv_translate_nlerr(-err);
        }
        goto fail;
    }

  done:;
    rtnl_link_put(rtnl_link_patch);
    rtnl_link_put(rtnl_link);
    return err;
  fail:;
    goto done;
}

///////////////////////////////////////////////////////////////////////////////
#endif
