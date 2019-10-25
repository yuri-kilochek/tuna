#include <tuna/priv/linux.h>

///////////////////////////////////////////////////////////////////////////////

TUNA_PRIV_API
unsigned
tuna_get_index(tuna_ref const *ref) {
    return ref->index;
}

int
tuna_unchecked_bind_by_index(tuna_ref *ref, unsigned index) {
    tuna_unbind(ref);
    ref->index = index;
    return 0;
}

TUNA_PRIV_API
tuna_error
tuna_bind_by_index(tuna_ref *ref, unsigned index) {
    // TODO: check if the device with this exists

    return tuna_unchecked_bind_by_index(ref, index);
}
