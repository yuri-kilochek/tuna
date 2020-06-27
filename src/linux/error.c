#include <tuna/priv/linux/error.h>

#include <errno.h>

///////////////////////////////////////////////////////////////////////////////

tuna_error
tuna_translate_errno(void) {
    switch (errno) {
    case 0:
        return 0;
    case ENXIO:
    case ENODEV:
        return TUNA_ERROR_INTERFACE_LOST;
    case EBUSY:
        return TUNA_ERROR_INTERFACE_BUSY;
    case EPERM:
    case EACCES:
        return TUNA_ERROR_FORBIDDEN;
    case ENOMEM:
    case ENOBUFS:
        return TUNA_ERROR_OUT_OF_MEMORY;
    case EMFILE:
    case ENFILE:
        return TUNA_ERROR_TOO_MANY_HANDLES;
    default:
        return TUNA_IMPL_ERROR_UNKNOWN;
    }
}

