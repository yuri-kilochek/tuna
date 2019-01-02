#ifndef TUNA_DETAIL_INCLUDED_DETAIL_FN_ALIAS
#define TUNA_DETAIL_INCLUDED_DETAIL_FN_ALIAS

#define TUNA_DETAIL_FN_ALIAS(...) \
    noexcept(noexcept(__VA_ARGS__)) \
    -> decltype(__VA_ARGS__) \
    { return __VA_ARGS__; } \
/**/

#endif
