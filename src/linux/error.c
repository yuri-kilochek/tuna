#include <tuna/priv/linux.h>

///////////////////////////////////////////////////////////////////////////////

int
tuna_translate_sys_error(int errnum) {
    switch (errnum) {
    case 0:
        return 0;
    default:
        return TUNA_UNEXPECTED;
    case ENXIO:
    case ENODEV:
        return TUNA_DEVICE_LOST;
    case EPERM:
    case EACCES:
        return TUNA_FORBIDDEN;
    case ENOMEM:
    case ENOBUFS:
        return TUNA_OUT_OF_MEMORY;
    case EMFILE:
    case ENFILE:
        return TUNA_TOO_MANY_HANDLES;
    case EBUSY:
        return TUNA_DEVICE_BUSY;
    }
}
