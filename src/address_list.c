#include <tuna/priv.h>

#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////

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
