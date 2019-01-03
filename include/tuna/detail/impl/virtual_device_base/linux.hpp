#ifndef TUNA_DETAIL_INCLUDED_DETAIL_IMPL_VIRTUAL_DEVICE_BASE_LINUX
#define TUNA_DETAIL_INCLUDED_DETAIL_IMPL_VIRTUAL_DEVICE_BASE_LINUX

#include <tuna/detail/complete_op.hpp>

#include <boost/scope_exit.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/post.hpp>

#include <utility>
#include <cerrno>

#include <fcntl.h>
#include <unistd.h>
#include <linux/if.h>
#include <linux/if_tun.h>

namespace tuna {
namespace detail {
///////////////////////////////////////////////////////////////////////////////

struct virtual_device_base
: boost::asio::posix::stream_descriptor
{
    using stream_descriptor::stream_descriptor;

    auto is_installed()
    const
    -> bool
    { return is_open(); }

    void install(boost::system::error_code& ec)
    {
        if (is_installed()) {
            ec.clear();
            return;
        }

        int fd = ::open("/dev/net/tun", O_RDWR);
        if (fd == -1) {
            ec.assign(errno, boost::system::system_category());
            return;
        }
        BOOST_SCOPE_EXIT_ALL(&) {
            if (fd == -1) { return; }
            ::close(fd);
        };

        ::ifreq ifr = {};
        ifr.ifr_flags = IFF_TUN;
        if (::ioctl(fd, TUNSETIFF, reinterpret_cast<char*>(&ifr)) == -1) {
            ec.assign(errno, boost::system::system_category());
            return;
        }

        assign(fd);
        fd = -1;
    }

    template <typename Handler>
    void async_install(Handler&& handler)
    {
        boost::system::error_code ec;
        if (!is_installed()) { install(ec); }
        boost::asio::post(detail::make_complete_op(
            *this, std::forward<Handler>(handler), ec));
    }

    void uninstall(boost::system::error_code& ec)
    { close(ec); }

    template <typename Handler>
    void async_uninstall(Handler&& handler)
    {
        boost::system::error_code ec;
        close(ec);
        boost::asio::post(detail::make_complete_op(
            *this, std::forward<Handler>(handler), ec));
    }


    auto is_active()
    const
    -> bool
    { 
        // TODO
        return false;
    }

    void activate(boost::system::error_code& ec)
    { /* TODO */ }

    template <typename Handler>
    void async_activate(Handler&& handler)
    { /* TODO */ }

    void deactivate(boost::system::error_code& ec)
    { /* TODO */ }

    template <typename Handler>
    void async_deactivate(Handler&& handler)
    { /* TODO */ }
};

///////////////////////////////////////////////////////////////////////////////
} // namespace detail
} // namespace tuna

#endif
