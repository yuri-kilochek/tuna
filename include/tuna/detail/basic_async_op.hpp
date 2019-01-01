#ifndef TUNA_DETAIL_INCLUDED_DETAIL_BASIC_ASYNC_OP
#define TUNA_DETAIL_INCLUDED_DETAIL_BASIC_ASYNC_OP

#include <boost/asio/associated_allocator.hpp>
#include <boost/asio/associated_executor.hpp>

#include <utility>

namespace tuna::detail {
///////////////////////////////////////////////////////////////////////////////

template <typename IoObject, typename Handler>
struct basic_async_op
{
    basic_async_op(IoObject& io_object, Handler& handler)
    : io_object_(io_object)
    , handler_(std::move(handler))
    {}

    basic_async_op(basic_async_op&&) = default;

    using allocator_type = 
        boost::asio::associated_allocator_t<Handler,
            boost::asio::associated_allocator_t<IoObject>>;

    auto get_allocator()
    const noexcept
    -> allocator_type
    { return boost::asio::get_associated_allocator(handler_,
        boost::asio::get_associated_allocator(io_object_)); }

    using executor_type =
        boost::asio::associated_executor_t<Handler,
            boost::asio::associated_executor_t<IoObject>>;

    auto get_executor()
    const noexcept
    -> executor_type
    { return boost::asio::get_associated_executor(handler_,
        boost::asio::get_associated_executor(io_object_)); }

protected:
    IoObject& io_object_;
    Handler handler_;
};

///////////////////////////////////////////////////////////////////////////////
}

#endif
