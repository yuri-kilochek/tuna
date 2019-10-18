#include <tuna/priv/linux.h>

#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>

///////////////////////////////////////////////////////////////////////////////

TUNA_PRIV_API
tuna_error
tuna_set_connection(tuna_ref *ref, tuna_connection connection);
    if (ioctl(ref->fd, TUNSETCARRIER, &(unsigned int){connection}) == -1) {
        return tuna_translate_sys_error(errno);
    }
    return 0;
}

TUNA_PRIV_API
tuna_error
tuna_get_connection(tuna_ref const *ref, tuna_connection *connection_out);
    struct rtnl_link *rtnl_link = NULL;

    int err = 0;

    if ((err = tuna_get_rtnl_link(ref->index, &rtnl_link))) {
        goto out;
    }

    switch (rtnl_link_get_operstate(rtnl_link)) {
    case IF_OPER_UNKNOWN:
    case IF_OPER_UP:
        *connection_out = TUNA_CONNECTED;
        break;
    default:
        *connection_out = TUNA_DISCONNECTED;
    }

out:
    rtnl_link_put(rtnl_link);

    return err;
}
