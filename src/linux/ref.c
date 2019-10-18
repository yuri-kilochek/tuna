#include <tuna/priv/linux.h>

#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////

TUNA_PRIV_API
tuna_error
tuna_get_ref(tuna_ref **ref_out) {
    tuna_ref *ref;
    if (!(ref = malloc(sizeof(*ref)))) {
        return tuna_translate_sys_error(errno);
    }
    *ref = (tuna_ref){
        .fd = -1,
    };

    *ref_out = ref;

    return 0;
}

TUNA_PRIV_API
void
tuna_free_ref(tuna_ref *ref) {
    if (ref) {
        tuna_unbind(ref);
        free(ref);
    }
}

TUNA_PRIV_API
tuna_error
tuna_bind_like(tuna_ref *ref, tuna_ref const *example_ref) {
    tuna_unbind(ref);
    ref->index = example_ref->index;
    return 0;
}

TUNA_PRIV_API
uint_fast8_t
tuna_is_bound(tuna_ref const *ref) {
    return !!ref->index;
}

TUNA_PRIV_API
void
tuna_unbind(tuna_ref *ref) {
    tuna_close(ref);
    ref->index = 0;
}
