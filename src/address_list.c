#include <tuna/priv/address_list.h>
#include <tuna/address_list.h>

#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////

tuna_error
tuna_allocate_address_list(size_t count, tuna_address_list **list_out) {
    tuna_address_list *list = NULL;

    tuna_error error = 0;

    if (!(list = malloc(sizeof(*list) + count * sizeof(*list->items)))) {
        error = TUNA_ERROR_OUT_OF_MEMORY;
        goto out;
    }
    for (list->count = 0; list->count < count; ++list->count) {
        list->items[list->count] = (tuna_address) {0};
    }

    *list_out = list; list = NULL;
out:
    tuna_free_address_list(list);

    return error;
}

size_t
tuna_get_address_count(tuna_address_list const *list) {
    return list->count;
}

tuna_address const *
tuna_get_address_at(tuna_address_list const *list, size_t position) {
    return &list->items[position];
}

void
tuna_free_address_list(tuna_address_list *list) {
    free(list);
}

