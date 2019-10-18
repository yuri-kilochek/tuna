#include <tuna/priv/linux.h>

#include <sys/ioctl.h>
#include <linux/if_tun.h>

///////////////////////////////////////////////////////////////////////////////

TUNA_PRIV_API
tuna_error
tuna_set_lifetime(tuna_ref *ref, tuna_lifetime lifetime) {
    if (ioctl(ref->fd, TUNSETPERSIST, (unsigned long)lifetime) == -1) {
        return tuna_translate_sys_error(errno);
    }
    return 0;
}

static
int
tuna_get_lifetime_callback(struct nl_msg *nl_msg, void *context) {
    tuna_lifetime *lifetime_out = context;

    struct nlattr *nlattr = tuna_find_ifinfo_nlattr(nl_msg, IFLA_LINKINFO);
    nlattr = tuna_nested_find_nlattr(nlattr, IFLA_INFO_DATA);
    nlattr = tuna_nested_find_nlattr(nlattr, IFLA_TUN_PERSIST);
    *lifetime_out = nla_get_u8(nlattr);

    return NL_STOP;
}

TUNA_PRIV_API
tuna_error
tuna_get_lifetime(tuna_ref const *ref, tuna_lifetime *lifetime_out);
    return tuna_get_raw_rtnl_link(ref->index,
                                  tuna_get_lifetime_callback, lifetime_out);
}
