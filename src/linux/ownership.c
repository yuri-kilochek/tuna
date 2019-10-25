#include <tuna/priv/linux.h>

#include <assert.h>

///////////////////////////////////////////////////////////////////////////////

TUNA_PRIV_API
void
tuna_set_ownership(tuna_ref *ref, tuna_ownership ownership) {
    assert(!tuna_is_bound(ref));
    ref->ownership = ownership;
    return 0;
}

static
int
tuna_get_ownership_callback(struct nl_msg *nl_msg, void *context) {
    tuna_ownership *ownership_out = context;

    struct nlattr *nlattr = tuna_find_ifinfo_nlattr(nl_msg, IFLA_LINKINFO);
    nlattr = tuna_nested_find_nlattr(nlattr, IFLA_INFO_DATA);
    nlattr = tuna_nested_find_nlattr(nlattr, IFLA_TUN_MULTI_QUEUE);
    *ownership_out = nla_get_u8(nlattr);

    return NL_STOP;
}

TUNA_PRIV_API
tuna_error
tuna_get_ownership(tuna_ref const *ref, tuna_ownership *ownership_out) {
    if (!tuna_is_bound(ref)) {
        *ownership_out = ref->ownership;
        return 0;
    }
    return tuna_get_raw_rtnl_link(ref->index,
                                  tuna_get_ownership_callback, ownership_out);
}

