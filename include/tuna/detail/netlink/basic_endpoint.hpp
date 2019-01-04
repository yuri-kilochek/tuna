#ifndef TUNA_DETAIL_INCLUDED_DETAIL_NETLINK_BASIC_ENDPOINT
#define TUNA_DETAIL_INCLUDED_DETAIL_NETLINK_BASIC_ENDPOINT

#include <tuna/detail/netlink/address.hpp>

#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/asio/generic/basic_endpoint.hpp>

#include <linux/netlink.h>
#include <sys/socket.h>

namespace tuna {
namespace detail {
namespace netlink {
///////////////////////////////////////////////////////////////////////////////

template <typename Protocol>
struct basic_endpoint
{
    using protocol_type = Protocol;

    auto protocol()
    const
    -> protocol_type
    { return {}; }


    auto address()
    const
    -> netlink::address
    {
        netlink::address address;
        if (nl_.nl_groups == 0) {
            address.port_id(nl_.nl_pid);
        } else {
            address.group_mask(nl_.nl_groups);
        }
        return address;
    }

    void address(netlink::address address)
    {
        if (address.is_unicast()) {
            nl_.nl_pid = address.port_id();
            nl_.nl_groups = 0;
        } else {
            nl_.nl_pid = 0;
            nl_.nl_groups = address.group_mask();
        }
    }


    explicit
    basic_endpoint(netlink::address address)
    { this->address(address); }

    basic_endpoint()
    = default;


    auto capacity()
    const
    -> std::size_t
    { return sizeof(nl_); }

    auto size()
    const
    -> std::size_t
    { return sizeof(nl_); }

    void resize(std::size_t new_size)
    {
        if (new_size != size()) {
            throw boost::system::system_error({
                boost::system::errc::invalid_argument});
        }
    }

    using data_type = 
        typename boost::asio::generic::basic_endpoint<Protocol>::data_type;

    auto data()
    const
    -> data_type const*
    { return reinterpret_cast<data_type const*>(&nl_); }

    auto data()
    -> data_type*
    { return reinterpret_cast<data_type*>(&nl_); }

private:
    ::sockaddr_nl nl_{AF_NETLINK};
};

///////////////////////////////////////////////////////////////////////////////
} // namespace netlink
} // namespace detail
} // namespace tuna

#endif
