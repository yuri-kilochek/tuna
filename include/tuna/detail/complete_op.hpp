#ifndef TUNA_DETAIL_INCLUDED_DETAIL_COMPLETE_OP
#define TUNA_DETAIL_INCLUDED_DETAIL_COMPLETE_OP

#include <tuna/detail/op_base.hpp>

// TODO: >=C++17: remove
#include <tuna/detail/apply.hpp>

#include <utility>
#include <tuple>

namespace tuna {
namespace detail {
///////////////////////////////////////////////////////////////////////////////

template <typename IoObject, typename Handler, typename... Results>
struct complete_op
: op_base<IoObject, Handler>
{
    std::tuple<Results...> results;

    void operator()()
    {
        // TODO: >=C++17: replace with std::apply
        apply(std::forward<Handler>(this->handler), std::move(results));
    }
};

template <typename IoObject, typename Handler, typename... Results>
auto make_complete_op_impl(IoObject& io_object,
    Handler&& handler, std::tuple<Results...>&& results)
TUNA_DETAIL_FN_ALIAS(complete_op<IoObject, Handler, Results...>{
    io_object, std::forward<Handler>(handler), std::move(results)})

template <typename IoObject, typename Handler, typename... Results>
auto make_complete_op(IoObject& io_object,
    Handler&& handler, Results&&... results)
TUNA_DETAIL_FN_ALIAS(make_complete_op_impl(
    io_object, std::forward<Handler>(handler),
    std::make_tuple(std::forward<Results>(results)...)))

///////////////////////////////////////////////////////////////////////////////
} // namespace detail
} // namespace tuna

#endif
