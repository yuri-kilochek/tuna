#include <tuna/priv/linux.h>

///////////////////////////////////////////////////////////////////////////////

int
tuna_translate_nl_error(int err) {
    switch (err) {
    case 0:
        return 0;
    default:
        return TUNA_UNEXPECTED;
    case NLE_NODEV:
    case NLE_OBJ_NOTFOUND:
        return TUNA_DEVICE_LOST;
    case NLE_PERM:
    case NLE_NOACCESS:
        return TUNA_FORBIDDEN;
    case NLE_NOMEM:
        return TUNA_OUT_OF_MEMORY;
    case NLE_BUSY:
        return TUNA_DEVICE_BUSY;
    }
}

int
tuna_open_nl_sock(struct nl_sock **nl_sock_out) {
    struct nl_sock *nl_sock = NULL;

    int err = 0;

    if (!(nl_sock = nl_socket_alloc())) {
        err = TUNA_OUT_OF_MEMORY;
        goto out;
    }

    if ((err = nl_connect(nl_sock, NETLINK_ROUTE))) {
        err = tuna_translate_nl_error(-err);
        goto out;
    }

    *nl_sock_out = nl_sock; nl_sock = NULL;

out:
    nl_socket_free(nl_sock);

    return err;
}

int
tuna_get_raw_rtnl_link_via(struct nl_sock *nl_sock, int index,
                           nl_recvmsg_msg_cb_t callback, void *context)
{
    struct nl_cb *nl_cb = NULL;
    struct nl_msg *nl_msg = NULL;

    int err = 0;

    if (!(nl_cb = nl_cb_alloc(NL_CB_CUSTOM))) {
        err = TUNA_OUT_OF_MEMORY;
        goto out;
    }

    if ((err = nl_cb_set(nl_cb, NL_CB_VALID, NL_CB_CUSTOM, callback, context))
     || (err = rtnl_link_build_get_request(index, NULL, &nl_msg))
     || (err = nl_send_auto(nl_sock, nl_msg)) < 0
     || (err = nl_recvmsgs(nl_sock, nl_cb)))
    {
        err = tuna_translate_nl_error(-err);
        goto out;
    }

out:
    nlmsg_free(nl_msg);
    nl_cb_put(nl_cb);

    return err;
}

int
tuna_get_raw_rtnl_link(int index,
                       nl_recvmsg_msg_cb_t callback, void *context)
{
    struct nl_sock *nl_sock = NULL;

    int err = 0;

    if ((err = tuna_open_nl_sock(&nl_sock)) ||
        (err = tuna_get_raw_rtnl_link_via(nl_sock, index, callback, context)))
    { goto out; }

out:
    nl_socket_free(nl_sock);

    return err;
}

struct nlattr *
tuna_find_ifinfo_nlattr(struct nl_msg *nl_msg, int type) {
    return nlmsg_find_attr(nlmsg_hdr(nl_msg), sizeof(struct ifinfomsg), type);
}

struct nlattr *
tuna_nested_find_nlattr(struct nlattr *nlattr, int type) {
    return nla_find(nla_data(nlattr), nla_len(nlattr), type);
}

int
tuna_get_rtnl_link_via(struct nl_sock *nl_sock, int index,
                       struct rtnl_link **rtnl_link_out)
{
    int err = rtnl_link_get_kernel(nl_sock, index, NULL, rtnl_link_out);
    if (err) { return tuna_translate_nl_error(-err); }
    return 0;
}

int
tuna_get_rtnl_link(int index, struct rtnl_link **rtnl_link_out) {
    struct nl_sock *nl_sock = NULL;
    struct rtnl_link *rtnl_link = NULL;

    int err = 0;

    if ((err = tuna_open_nl_sock(&nl_sock))
     || (err = tuna_get_rtnl_link_via(nl_sock, index, &rtnl_link)))
    { goto out; }

    *rtnl_link_out = rtnl_link; rtnl_link = NULL;

out:
    rtnl_link_put(rtnl_link);
    nl_socket_free(nl_sock);

    return err;
}

int
tuna_allocate_rtnl_link(struct rtnl_link **rtnl_link_out) {
    struct rtnl_link *rtnl_link = rtnl_link_alloc();
    if (!rtnl_link) { return TUNA_OUT_OF_MEMORY; }
    *rtnl_link_out = rtnl_link;
    return 0;
}

int
tuna_change_rtnl_link_via(struct nl_sock *nl_sock,
                          struct rtnl_link *rtnl_link,
                          struct rtnl_link *rtnl_link_changes)
{
    int err = rtnl_link_change(nl_sock, rtnl_link, rtnl_link_changes, 0);
    if (err) { return tuna_translate_nl_error(-err); }
    return 0;
}

int
tuna_change_rtnl_link_flags_via(struct nl_sock *nl_sock, int index,
                                void (*change)(struct rtnl_link *, unsigned),
                                unsigned flags, unsigned *old_flags_out)
{
    struct rtnl_link *rtnl_link = NULL;
    struct rtnl_link *rtnl_link_changes = NULL;

    int err = 0;

    if ((err = tuna_get_rtnl_link_via(nl_sock, index, &rtnl_link))
     || (err = tuna_allocate_rtnl_link(&rtnl_link_changes))
     || (change(rtnl_link_changes, flags), 0)
     || (err = tuna_change_rtnl_link_via(nl_sock,
                                         rtnl_link, rtnl_link_changes)))
    { goto out; }

    if (old_flags_out) {
        *old_flags_out = rtnl_link_get_flags(rtnl_link);
    }

out:
    rtnl_link_put(rtnl_link_changes);
    rtnl_link_put(rtnl_link);

    return err;
}
