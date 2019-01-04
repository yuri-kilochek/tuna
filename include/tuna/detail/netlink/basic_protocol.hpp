#ifndef TUNA_DETAIL_INCLUDED_DETAIL_NETLINK_BASIC_PROTOCOL
#define TUNA_DETAIL_INCLUDED_DETAIL_NETLINK_BASIC_PROTOCOL

#include <tuna/detail/netlink/basic_endpoint.hpp>

#include <boost/asio/basic_datagram_socket.hpp>

#include <linux/netlink.h>
#include <sys/socket.h>

namespace tuna {
namespace detail {
namespace netlink {
///////////////////////////////////////////////////////////////////////////////

template <int Protocol>
struct basic_protocol
{
    using endpoint = basic_endpoint<basic_protocol>;
    using socket = boost::asio::basic_datagram_socket<basic_protocol>;

    auto family()
    const
    -> int
    { return AF_NETLINK; }

    auto type()
    const
    -> int
    { return SOCK_DGRAM; }

    auto protocol()
    const
    -> int
    { return Protocol; }
};

///////////////////////////////////////////////////////////////////////////////
} // namespace netlink
} // namespace detail
} // namespace tuna

#endif
