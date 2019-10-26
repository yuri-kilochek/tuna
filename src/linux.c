#include <tuna/priv/linux.h>

#include <string.h>
#include <assert.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>

///////////////////////////////////////////////////////////////////////////////

TUNA_PRIV_API
tuna_error
tuna_open(tuna_ref *ref, tuna_ownership ownership) {
    assert(!tuna_is_open(ref));

    struct nl_sock *nl_sock = NULL;
    struct rtnl_link *rtnl_link = NULL;
    int fd = -1;

    int err = 0;

    if ((err = tuna_open_nl_sock(&nl_sock))) {
        goto out;
    }

    struct ifreq ifr = {
        .ifr_flags = IFF_TUN | IFF_NO_PI | (IFF_MULTI_QUEUE & -ownership),
    };
    if (tuna_is_bound(ref)) {
        if ((err = tuna_get_rtnl_link_via(nl_sock, ref->index, &rtnl_link))) {
            goto out;
        }
        strcpy(ifr.ifr_name, rtnl_link_get_name(rtnl_link));
    }

    if ((fd = open("/dev/net/tun", O_RDWR)) == -1
     || ioctl(fd, TUNSETIFF, &ifr) == -1)
    {
        err = tuna_translate_sys_error(errno);
        goto out;
    }

    // XXX: Since we only get the interface's name that can be changed by any
    // priviledged process and there appears to be no `TUNGETIFINDEX` or other
    // machanism to obtain interface index directly from file descriptor,
    // here we have a potential race condition.
get_index:
    if (ioctl(nl_socket_get_fd(nl_sock), SIOCGIFINDEX, &ifr) == -1) {
        // XXX: The following handles the case when it's just this interface
        // that has been renamed, however if the freed name has then been
        // assigned to another interface, then we'll incorrectly get that
        // interface's index and no error. We can then attempt to refetch the
        // name via `TUNGETIFF` and check if they match, however it is possible
        // that before we can do that, our interface is renamed back to its
        // original name, causing the check to incorrectly succeed. Since there
        // appears to be no way to completely rule out a race, we content
        // ourselves with only handling the basic case, judging the rest
        // extremely unlikely to ever happen.
        if (errno == ENODEV && ioctl(fd, TUNGETIFF, &ifr) != -1) {
            goto get_index;
        }
        err = tuna_translate_sys_error(errno);
        goto out;
    }
    unsigned index = ifr.ifr_ifindex;

    // Even assuming the interface's index has been determined correctly, if
    // the interface has been deleted or renamed just before `TUNSETIFF`, then
    // `TUNSETIFF` will succeed and create a new interface instead of attaching
    // to an existing one. At least we can detect this afterwards, since the
    // new interface will have a different index.
    if (tuna_is_bound(ref) && index != ref->index) {
        err = TUNA_DEVICE_LOST;
        goto out;
    }

    // XXX: It is possible for `tuna_set_name` to fail in way that will leave
    // the persistent interface with `IFF_UP` unset. The following is required to
    // recover automatically, but will intefere with any external attempts to
    // unset `IFF_UP`.
    if ((err = tuna_change_rtnl_link_flags_via(nl_sock, ref->index,
                                               rtnl_link_set_flags,
                                               IFF_UP, NULL)))
    { goto out; }

    ref->index = index;
    ref->fd = fd; fd = -1;

out:
    if (fd != -1) { close(fd); }
    rtnl_link_put(rtnl_link);
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


