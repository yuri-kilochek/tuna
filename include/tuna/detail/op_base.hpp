#ifndef TUNA_DETAIL_INCLUDED_DETAIL_OP_BASE
#define TUNA_DETAIL_INCLUDED_DETAIL_OP_BASE

#include <tuna/detail/fn_alias.hpp>

#include <boost/asio/associated_allocator.hpp>
#include <boost/asio/associated_executor.hpp>

namespace tuna {
namespace detail {
///////////////////////////////////////////////////////////////////////////////

template <typename IoObject, typename Handler>
struct op_base
{
    IoObject& io_object;
    Handler handler;

    auto get_allocator()
    const
    TUNA_DETAIL_FN_ALIAS(boost::asio::get_associated_allocator(handler))

    using allocator_type = decltype(get_allocator());

    auto get_executor()
    const
    TUNA_DETAIL_FN_ALIAS(boost::asio::get_associated_executor(
        handler, io_object.get_executor()))

    using executor_type = decltype(get_executor());
};

///////////////////////////////////////////////////////////////////////////////
} // namespace detail
} // namespace tuna

#endif
