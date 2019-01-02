#ifndef TUNA_DETAIL_INCLUDED_VIRTUAL_DEVICE
#define TUNA_DETAIL_INCLUDED_VIRTUAL_DEVICE

#include <tuna/detail/virtual_device_base.hpp>

#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/io_context.hpp>

#include <utility>

namespace tuna {
///////////////////////////////////////////////////////////////////////////////

struct virtual_device
: private detail::virtual_device_base
{
    explicit
    virtual_device(boost::asio::io_context& io_context)
    : virtual_device_base(io_context)
    {}

    using virtual_device_base::native_handle_type;
    using virtual_device_base::native_handle;

    using virtual_device_base::executor_type;
    using virtual_device_base::get_executor;


    using virtual_device_base::is_installed;

    using virtual_device_base::install;

    void install()
    {
        boost::system::error_code ec;
        install(ec);
        if (ec) { throw boost::system::system_error(ec); }
    }

    template <typename CompletionToken,
        typename Signature = void(boost::system::error_code),
        typename Completion =
            boost::asio::async_completion<CompletionToken, Signature>>
    auto async_install(CompletionToken&& token)
    -> decltype(std::declval<Completion&>().result.get())
    {
        Completion completion(token);
        virtual_device_base::async_install(
            std::forward<decltype(completion.completion_handler)>(
                completion.completion_handler));
        return completion.result.get();
    }

    using virtual_device_base::uninstall;

    void uninstall()
    {
        boost::system::error_code ec;
        uninstall(ec);
        if (ec) { throw boost::system::system_error(ec); }
    }

    template <typename CompletionToken,
        typename Signature = void(boost::system::error_code),
        typename Completion =
            boost::asio::async_completion<CompletionToken, Signature>>
    auto async_uninstall(CompletionToken&& token)
    -> decltype(std::declval<Completion&>().result.get())
    {
        Completion completion(token);
        virtual_device_base::async_uninstall(
            std::forward<decltype(completion.completion_handler)>(
                completion.completion_handler));
        return completion.result.get();
    }


    using virtual_device_base::is_active;

    using virtual_device_base::activate;

    void activate()
    {
        boost::system::error_code ec;
        activate(ec);
        if (ec) { throw boost::system::system_error(ec); }
    }

    template <typename CompletionToken,
        typename Signature = void(boost::system::error_code),
        typename Completion =
            boost::asio::async_completion<CompletionToken, Signature>>
    auto async_activate(CompletionToken&& token)
    -> decltype(std::declval<Completion&>().result.get())
    {
        Completion completion(token);
        virtual_device_base::async_activate(
            std::forward<decltype(completion.completion_handler)>(
                completion.completion_handler));
        return completion.result.get();
    }

    using virtual_device_base::deactivate;

    void deactivate()
    {
        boost::system::error_code ec;
        deactivate(ec);
        if (ec) { throw boost::system::system_error(ec); }
    }

    template <typename CompletionToken,
        typename Signature = void(boost::system::error_code),
        typename Completion =
            boost::asio::async_completion<CompletionToken, Signature>>
    auto async_deactivate(CompletionToken&& token)
    -> decltype(std::declval<Completion&>().result.get())
    {
        Completion completion(token);
        virtual_device_base::async_deactivate(
            std::forward<decltype(completion.completion_handler)>(
                completion.completion_handler));
        return completion.result.get();
    }

    using virtual_device_base::read_some;
    using virtual_device_base::async_read_some;

    using virtual_device_base::write_some;
    using virtual_device_base::async_write_some;

    using virtual_device_base::cancel;
};

///////////////////////////////////////////////////////////////////////////////
} // namespace tuna

#endif
