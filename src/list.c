#include <tuna/priv.h>

#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////

int
tuna_allocate_list(size_t count, tuna_list **list_out) {
    tuna_list *list;
    if (!(list = malloc(sizeof(*list) + count * sizeof(*list->items)))) {
        return TUNA_OUT_OF_MEMORY;
    }

    list->count = count;
    for (size_t i = 0; i < count; ++i) {
        list->items[i] = NULL;
    }

    *list_out = list;

    return 0;
}

TUNA_PRIV_API
void
tuna_free_list(tuna_list *list) {
    if (list) {
        for (size_t i = list->count; i--;) {
            tuna_free_ref(list->items[i]);
        }
        free(list);
    }
}

TUNA_PRIV_API
size_t
tuna_get_count(tuna_list const *list) {
    return list->count;
}

TUNA_PRIV_API
tuna_error
tuna_bind_at(tuna_ref *ref, tuna_list const *list, size_t position) {
    return tuna_bind_like(ref, list->items[position]);
}
