#include <tuna/priv.h>

#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////

int
tuna_allocate_ref_list(size_t count, tuna_ref_list **list_out) {
    tuna_ref_list *list;
    if (!(list = malloc(sizeof(*list) + count * sizeof(*list->items)))) {
        return tuna_translate_sys_error(errno);
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
tuna_free_ref_list(tuna_ref_list *list) {
    if (list) {
        for (size_t i = list->count - 1; i < list->count; --i) {
            tuna_free_ref(list->items[i]);
        }
        free(list);
    }
}

TUNA_PRIV_API
size_t
tuna_get_ref_count(tuna_ref_list const *list) {
    return list->count;
}

TUNA_PRIV_API
tuna_error
tuna_bind_like_at(tuna_ref *ref,
                  tuna_ref_list const *example_list, size_t example_position)
{
    return tuna_bind_like(ref, example_list->items[example_position]);
}
