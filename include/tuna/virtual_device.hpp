#ifndef TUNA_DETAIL_INCLUDED_VIRTUAL_DEVICE
#define TUNA_DETAIL_INCLUDED_VIRTUAL_DEVICE

#include <tuna/detail/virtual_device_service.hpp>

#include <boost/asio/basic_io_object.hpp>
#include <boost/system/error_code.hpp>

#include <utility>

namespace tuna {
///////////////////////////////////////////////////////////////////////////////

namespace detail {
    using virtual_device_base =
        boost::asio::basic_io_object<detail::virtual_device_service>;
}

struct virtual_device
: private detail::virtual_device_base
{
    explicit
    virtual_device(boost::asio::io_context& io_context)
    : detail::virtual_device_base(io_context)
    {}



    auto is_installed()
    const
    -> bool
    { return get_service().is_installed(get_implementation()); }


    void install()
    { get_service().install(get_implementation()); }

    void install(boost::system::error_code& ec)
    { get_service().install(get_implementation(), ec); }

    template <typename InstallHandler>
    auto async_install(InstallHandler&& handler)
    { return get_service().install(get_implementation(),
        std::forward<InstallHandler>(handler)); }


    void uninstall()
    { get_service().uninstall(get_implementation()); }

    void uninstall(boost::system::error_code& ec)
    { get_service().uninstall(get_implementation(), ec); }

    template <typename UninstallHandler>
    auto async_uninstall(UninstallHandler&& handler)
    { return get_service().async_uninstall(get_implementation(),
        std::forward<UninstallHandler>(handler)); }
    


    auto is_active()
    const
    -> bool
    { return get_service().is_active(get_implementation()); }


    void activate()
    { get_service().activate(get_implementation()); }

    void activate(boost::system::error_code& ec)
    { get_service().activate(get_implementation(), ec); }

    template <typename ActivateHandler>
    auto async_activate(ActivateHandler&& handler)
    { return get_service().async_activate(get_implementation(),
        std::forward<ActivateHandler>(handler)); }


    void deactivate()
    { get_service().deactivate(get_implementation()); }

    void deactivate(boost::system::error_code& ec)
    { get_service().deactivate(get_implementation(), ec); }

    template <typename DeactivateHandler>
    auto async_deactivate(DeactivateHandler&& handler)
    { return get_service().async_deactivate(get_implementation(),
        std::forward<DeactivateHandler>(handler)); }



    template <typename MutableBufferSequence>
    auto read_some(MutableBufferSequence const& buffers)
    -> std::size_t
    { return get_service().read_some(get_implementation(), buffers); }

    template <typename MutableBufferSequence>
    auto read_some(MutableBufferSequence const& buffers,
                   boost::system::error_code& ec)
    -> std::size_t
    { return get_service().read_some(get_implementation(), buffers, ec); }

    template <typename MutableBufferSequence, typename ReadHandler>
    auto async_read_some(MutableBufferSequence const& buffers,
                         ReadHandler&& handler)
    { return get_service().async_read_some(get_implementation(),
        buffers, std::forward<ReadHandler>(handler)); }


    template <typename ConstBufferSequence>
    auto write_some(ConstBufferSequence const& buffers)
    -> std::size_t
    { return get_service().write_some(get_implementation(), buffers); }

    template <typename ConstBufferSequence>
    auto write_some(ConstBufferSequence const& buffers,
                    boost::system::error_code& ec)
    -> std::size_t
    { return get_service().write_some(get_implementation(), buffers, ec); }

    template <typename ConstBufferSequence, typename WriteHandler>
    auto async_write_some(ConstBufferSequence const& buffers,
                          WriteHandler&& handler)
    { return get_service().async_write_some(get_implementation(),
        buffers, std::forward<WriteHandler>(handler)); }
};

///////////////////////////////////////////////////////////////////////////////
}

#endif
