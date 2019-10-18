#include <tuna/priv.h>

///////////////////////////////////////////////////////////////////////////////

TUNA_PRIV_API
tuna_version
tuna_get_runtime_version(void) {
    return TUNA_HEADER_VERSION;
}
