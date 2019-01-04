#ifndef TUNA_DETAIL_INCLUDED_DETAIL_NETLINK_ADDRESS
#define TUNA_DETAIL_INCLUDED_DETAIL_NETLINK_ADDRESS

#include <utility>
#include <exception>

#include <linux/netlink.h>

namespace tuna {
namespace detail {
namespace netlink {
///////////////////////////////////////////////////////////////////////////////

struct bad_address_access
: std::exception
{
    virtual
    auto what()
    const noexcept
    -> char const*
    { return "bad address access"; }
};

struct address
{
    auto is_unicast()
    const
    -> bool
    { return !is_multicast_; }

    using port_id_type = decltype(std::declval<::sockaddr_nl>().nl_pid);

    auto port_id()
    const
    -> port_id_type
    {
        if (!is_unicast()) { throw bad_address_access(); }
        return port_id_;
    }

    void port_id(port_id_type port_id)
    {
        port_id_ = port_id;
        is_multicast_ = false;
    }


    auto is_multicast()
    const
    -> bool
    { return is_multicast_; }

    using group_mask_type = decltype(std::declval<::sockaddr_nl>().nl_groups);

    auto group_mask()
    const
    -> group_mask_type
    {
        if (!is_multicast()) { throw bad_address_access(); }
        return group_mask_;
    }

    void group_mask(group_mask_type group_mask)
    {
        group_mask_ = group_mask;
        is_multicast_ = true;
    }


    address()
    = default;

private:
    bool is_multicast_ = false;
    union {
        port_id_type port_id_ = 0;
        group_mask_type group_mask_;
    };
};

inline
auto make_unicast_address(address::port_id_type port_id)
-> address
{
    netlink::address address;
    address.port_id(port_id);
    return address;
}

inline
auto make_multicast_address(address::group_mask_type group_mask)
-> address
{
    netlink::address address;
    address.group_mask(group_mask);
    return address;
}

///////////////////////////////////////////////////////////////////////////////
} // namespace netlink
} // namespace detail
} // namespace tuna

#endif
