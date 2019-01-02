#ifndef TUNA_DETAIL_INCLUDED_DETAIL_APPLY
#define TUNA_DETAIL_INCLUDED_DETAIL_APPLY

// TODO: >=C++17: delete this file

#include <tuple>

#if __cplusplus < 201703L
    #include <tuna/detail/fn_alias.hpp>

    #include <cstddef>
    #include <utility>
    #include <type_traits>
#endif

namespace tuna {
namespace detail {
///////////////////////////////////////////////////////////////////////////////

#if __cplusplus >= 201703L
    using std::apply;
#else
    template <std::size_t Count>
    struct apply_impl
    {
        template <typename Fn, typename Tuple, typename... Args>
        auto operator()(Fn&& fn, Tuple&& tuple, Args&&... args)
        TUNA_DETAIL_FN_ALIAS(apply_impl<Count - 1>{}(
            std::forward<Fn>(fn), std::forward<Tuple>(tuple),
            std::get<Count - 1>(std::forward<Tuple>(tuple)),
            std::forward<Args>(args)...))
    };

    template <>
    struct apply_impl<0>
    {
        template <typename Fn, typename Tuple, typename... Args>
        auto operator()(Fn&& fn, Tuple&&, Args&&... args)
        TUNA_DETAIL_FN_ALIAS(std::forward<Fn>(fn)(std::forward<Args>(args)...))
    };

    // Quick and dirty shim, does not try to be fully equivalent to std::apply.
    template <typename Fn, typename Tuple,
        typename BareTuple = typename std::remove_reference<Tuple>::type>
    auto apply(Fn&& fn, Tuple&& tuple)
    TUNA_DETAIL_FN_ALIAS(apply_impl<std::tuple_size<BareTuple>::value>{}(
        std::forward<Fn>(fn), std::forward<Tuple>(tuple)))
#endif

///////////////////////////////////////////////////////////////////////////////
} // namespace detail
} // namespace tuna

#endif
