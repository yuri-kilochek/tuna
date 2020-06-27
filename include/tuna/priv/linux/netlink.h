#ifndef TUNA_IMPL_INCL_PRIV_LINUX_NETLINK_H
#define TUNA_IMPL_INCL_PRIV_LINUX_NETLINK_H

#include <tuna/error.h>

#include <netlink/netlink.h>

///////////////////////////////////////////////////////////////////////////////

tuna_error
tuna_translate_nlerr(int err);

int
tuna_rtnl_socket_open(struct nl_sock **sock_out);

int
tuna_rtnl_link_get_raw(struct nl_sock *sock, int index,
                       struct nl_msg **msg_out);

struct nlattr *
tuna_nlmsg_find_ifinfo_attr(struct nl_msg *msg, int type);

struct nlattr *
tuna_nla_find_nested(struct nlattr *attr, int type);

///////////////////////////////////////////////////////////////////////////////

#endif

