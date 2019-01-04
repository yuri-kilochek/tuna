#ifndef TUNA_DETAIL_INCLUDED_DETAIL_NETLINK_ROUTE
#define TUNA_DETAIL_INCLUDED_DETAIL_NETLINK_ROUTE

#include <tuna/detail/netlink/basic_protocol.hpp>

#include <linux/netlink.h>

namespace tuna {
namespace detail {
namespace netlink {
///////////////////////////////////////////////////////////////////////////////

using route = basic_protocol<NETLINK_ROUTE>;

///////////////////////////////////////////////////////////////////////////////
} // namespace netlink
} // namespace detail
} // namespace tuna

#endif
