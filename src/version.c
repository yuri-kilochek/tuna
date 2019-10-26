#include <tuna/priv.h>

///////////////////////////////////////////////////////////////////////////////

TUNA_PRIV_API
tuna_version
tuna_get_linked_version(void) {
    return TUNA_INCLUDED_VERSION;
}
