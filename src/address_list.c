#include <tuna/priv.h>

#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////

int
tuna_allocate_address_list(size_t count, tuna_address_list **list_out) {
    tuna_address_list *list;
    if (!(list = malloc(sizeof(*list) + count * sizeof(*list->items)))) {
        return TUNA_OUT_OF_MEMORY;
    }

    list->count = count;

    *list_out = list;

    return 0;
}

TUNA_PRIV_API
void
tuna_free_address_list(tuna_address_list *list) {
    free(list);
}

TUNA_PRIV_API
size_t
tuna_get_address_count(tuna_address_list const *list) {
    return list->count;
}

TUNA_PRIV_API
tuna_address
tuna_get_address_at(tuna_address_list const *list, size_t position) {
    return list->items[position];
}
