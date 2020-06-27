#include <tuna/priv/linux/netlink.h>

#include <netlink/route/link.h>

///////////////////////////////////////////////////////////////////////////////

tuna_error
tuna_translate_nlerr(int err) {
    switch (err) {
    case 0:
        return 0;
    case -NLE_NODEV:
    case -NLE_OBJ_NOTFOUND:
        return TUNA_ERROR_INTERFACE_LOST;
    case -NLE_BUSY:
        return TUNA_ERROR_INTERFACE_BUSY;
    case -NLE_PERM:
    case -NLE_NOACCESS:
        return TUNA_ERROR_FORBIDDEN;
    case -NLE_NOMEM:
        return TUNA_ERROR_OUT_OF_MEMORY;
    default:
        return TUNA_IMPL_ERROR_UNKNOWN;
    }
}

int
tuna_rtnl_socket_open(struct nl_sock **sock_out) {
    struct nl_sock *sock = NULL;

    int err = 0;

    if (!(sock = nl_socket_alloc())) {
        err = -NLE_NOMEM;
        goto out;
    }
    if (0 > (err = nl_connect(sock, NETLINK_ROUTE))) {
        goto out;
    }

    *sock_out = sock; sock = NULL;
out:
    nl_socket_free(sock);

    return err;
}


static
int
tuna_rtnl_link_get_raw_cb(struct nl_msg *msg, void *ctx) {
    struct nl_msg **msg_out = ctx;

    nlmsg_get((*msg_out = msg));

    return NL_STOP;
}

int
tuna_rtnl_link_get_raw(struct nl_sock *sock, int index,
                       struct nl_msg **msg_out)
{
    struct nl_cb *cb = NULL;
    struct nl_msg *msg = NULL;

    int err = 0;

    if (0 > (err = rtnl_link_build_get_request(index, NULL, &msg))
     || 0 > (err = nl_send_auto(sock, msg)))
    { goto out; }

    nlmsg_free(msg); msg = NULL;

    if (!(cb = nl_cb_alloc(NL_CB_CUSTOM))) {
        err = -NLE_NOMEM;
        goto out;
    }
    if (0 > (err = nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM,
                             tuna_rtnl_link_get_raw_cb, &msg))
     || 0 > (err = nl_recvmsgs(sock, cb)))
    { goto out; }

    *msg_out = msg; msg = NULL;
out:
    nlmsg_free(msg);
    nl_cb_put(cb);

    return err;
}

struct nlattr *
tuna_nlmsg_find_ifinfo_attr(struct nl_msg *msg, int type) {
    return nlmsg_find_attr(nlmsg_hdr(msg), sizeof(struct ifinfomsg), type);
}

struct nlattr *
tuna_nla_find_nested(struct nlattr *attr, int type) {
    return nla_find(nla_data(attr), nla_len(attr), type);
}

