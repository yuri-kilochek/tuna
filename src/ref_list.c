#include <tuna/priv/ref_list.h>
#include <tuna/ref_list.h>

#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////

tuna_error
tuna_allocate_ref_list(size_t count, tuna_ref_list **list_out) {
    tuna_ref_list *list = NULL;

    tuna_error error = 0;

    if (!(list = malloc(sizeof(*list) + count * sizeof(*list->items)))) {
        error = TUNA_ERROR_OUT_OF_MEMORY;
        goto out;
    }
    for (list->count = 0; list->count < count; ++list->count) {
        if ((error = tuna_allocate_ref(&list->items[list->count]))) {
            goto out;
        }
    }

    *list_out = list; list = NULL;
out:
    tuna_free_ref_list(list);

    return error;
}

size_t
tuna_get_ref_count(tuna_ref_list const *list) {
    return list->count;
}

tuna_ref const *
tuna_get_ref_at(tuna_ref_list const *list, size_t position) {
    return list->items[position];
}

void
tuna_free_ref_list(tuna_ref_list *list) {
    if (!list) { return; }
    while (list->count > 0) {
        tuna_free_ref(list->items[--list->count]);
    }
    free(list);
}

