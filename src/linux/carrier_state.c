#include <tuna/priv/linux.h>

#include <assert.h>

#include <sys/ioctl.h>
#include <linux/if_tun.h>

///////////////////////////////////////////////////////////////////////////////

TUNA_PRIV_API
tuna_error
tuna_set_carrier_state(tuna_ref *ref, tuna_carrier_state state) {
    assert(tuna_is_open(ref));

    if (ioctl(ref->fd, TUNSETCARRIER, &(unsigned int){state}) == -1) {
        return tuna_translate_sys_error(errno);
    }
    return 0;
}

TUNA_PRIV_API
tuna_error
tuna_get_carrier_state(tuna_ref const *ref, tuna_carrier_state *state_out) {
    assert(tuna_is_bound(ref));

    struct rtnl_link *rtnl_link = NULL;

    int err = 0;

    if ((err = tuna_get_rtnl_link(ref->index, &rtnl_link))) {
        goto out;
    }

    *state_out = rtnl_link_get_carrier(rtnl_link);

out:
    rtnl_link_put(rtnl_link);

    return err;
}
