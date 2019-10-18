#include <tuna/priv/linux.h>

///////////////////////////////////////////////////////////////////////////////

TUNA_PRIV_API
tuna_io_handle
tuna_get_io_handle(tuna_ref const *ref) {
    return ref->fd;
}
