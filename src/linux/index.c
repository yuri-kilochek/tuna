#include <tuna/priv/linux.h>

///////////////////////////////////////////////////////////////////////////////

TUNA_PRIV_API
unsigned
tuna_get_index(tuna_ref const *ref) {
    return ref->index;
}

TUNA_PRIV_API
tuna_error
tuna_bind_by_index(tuna_ref *ref, unsigned index) {
    // TODO: check if it exists

    tuna_unbind(ref);
    ref->index = index;
    return 0;
}
