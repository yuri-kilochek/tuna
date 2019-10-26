#ifndef TUNA_PRIV_INCLUDE_GUARD_PRIV_LINUX_NETLINK
#define TUNA_PRIV_INCLUDE_GUARD_PRIV_LINUX_NETLINK

#include <tuna/priv.h>

#include <netlink/netlink.h>
#include <netlink/route/link.h>

///////////////////////////////////////////////////////////////////////////////

int
tuna_translate_nl_error(int err);

int
tuna_open_nl_sock(struct nl_sock **nl_sock_out);

int
tuna_get_raw_rtnl_link_via(struct nl_sock *nl_sock, int index,
                           nl_recvmsg_msg_cb_t callback, void *context);

int
tuna_get_raw_rtnl_link(int index,
                       nl_recvmsg_msg_cb_t callback, void *context);

struct nlattr *
tuna_find_ifinfo_nlattr(struct nl_msg *nl_msg, int type);

struct nlattr *
tuna_nested_find_nlattr(struct nlattr *nlattr, int type);

int
tuna_get_rtnl_link_via(struct nl_sock *nl_sock, int index,
                       struct rtnl_link **rtnl_link_out);

int
tuna_get_rtnl_link(int index, struct rtnl_link **rtnl_link_out);

int
tuna_allocate_rtnl_link(struct rtnl_link **rtnl_link_out);

int
tuna_change_rtnl_link_via(struct nl_sock *nl_sock,
                          struct rtnl_link *rtnl_link,
                          struct rtnl_link *rtnl_link_changes);

int
tuna_change_rtnl_link_flags_via(struct nl_sock *nl_sock, int index,
                                void (*change)(struct rtnl_link *, unsigned),
                                unsigned flags, unsigned *old_flags_out);

///////////////////////////////////////////////////////////////////////////////

#endif
