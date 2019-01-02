#ifndef TUNA_DETAIL_INCLUDED_DETAIL_IMPL_VIRTUAL_DEVICE_BASE_LINUX
#define TUNA_DETAIL_INCLUDED_DETAIL_IMPL_VIRTUAL_DEVICE_BASE_LINUX

#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/system/error_code.hpp>

namespace tuna::detail {
///////////////////////////////////////////////////////////////////////////////

struct virtual_device_base
: boost::asio::posix::stream_descriptor
{
    using stream_descriptor::stream_descriptor;

    auto is_installed()
    const
    -> bool
    { return is_open(); }

    void install(boost::system::error_code& error_code)
    {
        // TODO
    }

    template <typename InstallHandler>
    void async_install(InstallHandler&& handler)
    {
        
    }

    void uninstall(boost::system::error_code& error_code)
    {
        // TODO
    }

    template <typename InstallHandler>
    void async_uninstall(InstallHandler&& handler)
    {
        
    }


    auto is_active()
    const
    -> bool
    { return false; }

    void activate(boost::system::error_code& error_code)
    {
        // TODO
    }

    template <typename InstallHandler>
    void async_activate(InstallHandler&& handler)
    {
        
    }

    void deactivate(boost::system::error_code& error_code)
    {
        // TODO
    }

    template <typename InstallHandler>
    void async_deactivate(InstallHandler&& handler)
    {
        
    }
};

///////////////////////////////////////////////////////////////////////////////
}

#endif
