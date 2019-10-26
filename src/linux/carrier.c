#include <tuna/priv/linux.h>

#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>

#include <assert.h>

///////////////////////////////////////////////////////////////////////////////

TUNA_PRIV_API
tuna_error
tuna_set_carrier(tuna_ref *ref, tuna_carrier carrier);
    assert(tuna_is_open(ref));

    if (ioctl(ref->fd, TUNSETCARRIER, &(unsigned int){carrier}) == -1) {
        return tuna_translate_sys_error(errno);
    }
    return 0;
}

TUNA_PRIV_API
tuna_error
tuna_get_carrier(tuna_ref const *ref, tuna_carrier *carrier_out);
    assert(tuna_is_bound(ref));

    struct rtnl_link *rtnl_link = NULL;

    int err = 0;

    if ((err = tuna_get_rtnl_link(ref->index, &rtnl_link))) {
        goto out;
    }

    *carrier_out = rtnl_link_get_carrier(rtnl_link);

out:
    rtnl_link_put(rtnl_link);

    return err;
}
