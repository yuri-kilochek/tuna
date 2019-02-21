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
#include <netlink/route/addr.h>

// TODO: delete
#include <stdio.h>

///////////////////////////////////////////////////////////////////////////////

struct tuna_device {
    struct nl_sock *nl_sock;
    
    int fd;
    int ifindex;

    char name[IFNAMSIZ];
    
    tuna_address *addrs;
    size_t addrs_cap;
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
tuna_create_device(tuna_device **device) {
    struct rtnl_link *rtnl_link;

    int err = 0;

    tuna_device *dev = malloc(sizeof(*dev));
    if (!dev) {
        err = TUNA_OUT_OF_MEMORY;
        goto fail;
    }
    *dev = (tuna_device){.fd = -1};

    if (!(dev->nl_sock = nl_socket_alloc())) {
        err = TUNA_OUT_OF_MEMORY;
        goto fail;
    }

    errno = 0;
    switch ((err = nl_connect(dev->nl_sock, NETLINK_ROUTE))) {
      case 0:;
        break;
      case -NLE_FAILURE:;
        switch (errno) {
          case EMFILE:;
          case ENFILE:;
            err = TUNA_TOO_MANY_HANDLES;
            goto fail;
        }
        /* fallthrough */
      default:;
        err = tuna_priv_translate_nlerr(-err);
        goto fail;
    }

  open:;
    if ((dev->fd = open("/dev/net/tun", O_RDWR)) == -1) {
        err = tuna_priv_translate_syserr(errno);
        goto fail;
    }

    struct ifreq ifreq = {.ifr_flags = IFF_TUN | IFF_NO_PI};
    if (ioctl(dev->fd, TUNSETIFF, &ifreq) == -1) {
        err = tuna_priv_translate_syserr(errno);
        goto fail;
    }

    if ((err = rtnl_link_get_kernel(dev->nl_sock,
                                    0, ifreq.ifr_name, &rtnl_link)))
    {
        if (err == -NLE_OBJ_NOTFOUND) {
            close(dev->fd);
            goto open;
        }
        err = tuna_priv_translate_nlerr(-err);
        goto fail;
    }
    dev->ifindex = rtnl_link_get_ifindex(rtnl_link);

    *device = dev;
  done:;
    rtnl_link_put(rtnl_link);
    return err;
  fail:;
    tuna_destroy_device(dev);
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
tuna_get_name(tuna_device const *device, char const **name) {
    tuna_device *dev = (void *)device;

    struct rtnl_link *rtnl_link = NULL;

    int err = 0;
    
    if ((err = rtnl_link_get_kernel(dev->nl_sock,
                                    dev->ifindex, NULL, &rtnl_link)))
    {
        err = tuna_priv_translate_nlerr(-err);
        goto fail;
    }

    strcpy(dev->name, rtnl_link_get_name(rtnl_link));
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

tuna_error
tuna_get_addresses(tuna_device const *device,
                   tuna_address const **addresses, size_t *count)
{
    tuna_device *dev = (void *)device;

    struct nl_cache *nl_cache = NULL;

    int err = 0;

    if ((err = rtnl_addr_alloc_cache(dev->nl_sock, &nl_cache))) {
        err = tuna_priv_translate_nlerr(-err);
        goto fail;
    }

    size_t cnt = 0;
    for (struct nl_object *nl_object = nl_cache_get_first(nl_cache);
         nl_object; nl_object = nl_cache_get_next(nl_object))
    {
        if (cnt == dev->addrs_cap) {
            size_t addrs_cap = dev->addrs_cap * 5 / 3 + 1;
            tuna_address *addrs = realloc(dev->addrs, addrs_cap);
            if (!addrs) {
                err = TUNA_OUT_OF_MEMORY;
                goto fail;
            }
            dev->addrs = addrs;
            dev->addrs_cap = addrs_cap;
        }
        struct rtnl_addr *rtnl_addr = (void *)nl_object;
        
        if (rtnl_addr_get_ifindex(rtnl_addr) != dev->ifindex) { continue; }

        struct nl_addr *nl_addr = rtnl_addr_get_local(rtnl_addr);
        if (!nl_addr) { continue; }

        struct sockaddr_storage sockaddr;
        socklen_t socklen = sizeof(sockaddr);
        if ((err = nl_addr_fill_sockaddr(nl_addr,
                                         (void *)&sockaddr, &socklen)))
        {
            err = tuna_priv_translate_nlerr(-err);
            goto fail;
        }

        tuna_address *addr = dev->addrs + cnt;
        switch (sockaddr.ss_family) {
          case AF_INET: {
            uint32_t raw_addr = 
                ((struct sockaddr_in *)&sockaddr)->sin_addr.s_addr;

            addr->family = TUNA_IP4;

            addr->ip4.octets[0] = (raw_addr >> 24)       ;
            addr->ip4.octets[1] = (raw_addr >> 16) & 0xFF;
            addr->ip4.octets[2] = (raw_addr >>  8) & 0xFF;
            addr->ip4.octets[3] = (raw_addr      ) & 0xFF;

            addr->ip4.prefix_length = nl_addr_get_prefixlen(nl_addr);
            break;
          }
          case AF_INET6: {
            unsigned char *raw_addr = 
                ((struct sockaddr_in6 *)&sockaddr)->sin6_addr.s6_addr;

            addr->family = TUNA_IP6;

            addr->ip6.hextets[0] = raw_addr[ 0] << 8 | raw_addr[ 1];
            addr->ip6.hextets[1] = raw_addr[ 2] << 8 | raw_addr[ 3];
            addr->ip6.hextets[2] = raw_addr[ 4] << 8 | raw_addr[ 5];
            addr->ip6.hextets[3] = raw_addr[ 6] << 8 | raw_addr[ 7];
            addr->ip6.hextets[4] = raw_addr[ 8] << 8 | raw_addr[ 9];
            addr->ip6.hextets[5] = raw_addr[10] << 8 | raw_addr[11];
            addr->ip6.hextets[6] = raw_addr[12] << 8 | raw_addr[13];
            addr->ip6.hextets[7] = raw_addr[14] << 8 | raw_addr[15];

            addr->ip6.prefix_length = nl_addr_get_prefixlen(nl_addr);
            break;
          }
          default:
            continue;
        }

        ++cnt;
    }

    tuna_address *addrs = realloc(dev->addrs, cnt);
    if (addrs) {
        dev->addrs = addrs;
        dev->addrs_cap = cnt;
    }

    *addresses = dev->addrs;
    *count = cnt;
  done:;
    nl_cache_free(nl_cache);
    return err;
  fail:;
    goto done;
}

///////////////////////////////////////////////////////////////////////////////
#endif
