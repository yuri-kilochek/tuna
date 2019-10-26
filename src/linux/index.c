#include <tuna/priv/linux.h>

#include <net/if.h>

///////////////////////////////////////////////////////////////////////////////

TUNA_PRIV_API
unsigned
tuna_get_index(tuna_ref const *ref) {
    return ref->index;
}

void
tuna_unchecked_bind_by_index(tuna_ref *ref, unsigned index) {
    tuna_unbind(ref);
    ref->index = index;
}

TUNA_PRIV_API
tuna_error
tuna_bind_by_index(tuna_ref *ref, unsigned index) {
    char name[IF_NAMESIZE];
    if (!if_indextoname(index, name)) {
        if (errno == ENXIO) {
            return TUNA_DEVICE_NOT_FOUND;
        }
        return tuna_translate_sys_error(errno);
    }

    tuna_unchecked_bind_by_index(ref, index);

    return 0;
}
