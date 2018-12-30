#ifndef TUNA_DETAIL_INCLUDED_IMPL_VIRTUAL_DEVICE_LINUX
#define TUNA_DETAIL_INCLUDED_IMPL_VIRTUAL_DEVICE_LINUX

#include <tuna/detail/failers.hpp>

#include <boost/system/error_code.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
//#include <boost/asio/ip/network_v4.hpp>
//#include <boost/asio/ip/network_v6.hpp>
//#include <boost/asio/ip/udp.hpp>

#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>

namespace tuna {
///////////////////////////////////////////////////////////////////////////////

struct virtual_device
: private boost::asio::posix::stream_descriptor
{
    explicit virtual_device(boost::asio::io_context& io_context)
    : boost::asio::posix::stream_descriptor(io_context)
    {}

    void install(boost::system::error_code& ec)
    { install_impl(detail::error_code_setter(ec)); }

    void install()
    { install_impl(detail::system_error_thrower()); }

    auto is_installed()
    { return is_open(); }

    void uninstall(boost::system::error_code& ec)
    { close(ec); }

    void uninstall()
    { close(); }

    //boost::asio::ip::network_v4 ip_network_v4() const;
    //void network(boost::asio::ip::network_v4 const& network);

    //boost::asio::ip_network_v6 network_v6() const;
    //void network(boost::asio::network_v6 const& network);

    //void open(boost::system::error_code& ec) {
    //    boost::asio::posix::stream_descriptor
    //        prototype(get_io_context(), ::open("/dev/net/tun", O_RDWR));
    //    if (!prototype.is_open()) {
    //        ec.assign(errno, boost::system::system_category());
    //        return;
    //    }

    //    ::ifreq ifr = {};
    //    ifr.ifr_flags = IFF_TUN;
    //    if (::ioctl(prototype.native_handle(),
    //                TUNSETIFF,
    //                static_cast<void*>(&ifr)) == -1)
    //    {
    //        ec.assign(errno, boost::system::system_category());
    //        return;
    //    }

    //    boost::asio::ip::udp::socket dummy_socket(get_io_context());
    //    dummy_socket.open(boost::asio::ip::udp::endpoint().protocol(), ec);
    //    if (ec) { return; }

    //    if (::ioctl(dummy_socket.native_handle(),
    //                SIOCGIFFLAGS,
    //                static_cast<void*>(&ifr)) == -1)
    //    {
    //        ec.assign(errno, boost::system::system_category());
    //        return;
    //    }
    //    ifr.ifr_flags |= IFF_UP | IFF_LOWER_UP;
    //    if (::ioctl(dummy_socket.native_handle(),
    //                SIOCSIFFLAGS,
    //                static_cast<void*>(&ifr)) == -1)
    //    {
    //        ec.assign(errno, boost::system::system_category());
    //        return;
    //    }

    //    static_cast<boost::asio::posix::stream_descriptor&>(*this) = 
    //        std::move(prototype);
    //    ec.clear();
    //}

    //void open() {
    //    auto ec = boost::system::error_code();
    //    open(ec);
    //    if (ec) { throw boost::system::system_error(ec); }
    //}

private:

    template <typename Failer>
    void install_impl(Failer fail)
    {
        {
            int fd = ::open("/dev/net/tun", O_RDWR);
            if (fd == -1) {
                fail();
                return;
            }
            assign(fd);
        }

        {
            ::ifreq ifr = {};
            ifr.ifr_flags = IFF_TUN;
            if (::ioctl(native_handle(), TUNSETIFF, (char*)&ifr) == -1) {
                fail();
                return;
            }
        }
    }
};

///////////////////////////////////////////////////////////////////////////////
}

#endif
