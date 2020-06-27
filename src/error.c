#include <tuna/error.h>

///////////////////////////////////////////////////////////////////////////////

char const*
tuna_get_error_name(tuna_error error) {
    switch (error) {
    case 0:
        return "0";
    case TUNA_ERROR_UNSUPPORTED:
        return "unsupported";
    case TUNA_ERROR_INTERFACE_NOT_FOUND:
        return "interface not found";
    case TUNA_ERROR_INTERFACE_LOST:
        return "interface lost";
    case TUNA_ERROR_INTERFACE_BUSY:
        return "interface busy";
    case TUNA_ERROR_FORBIDDEN:
        return "forbidden";
    case TUNA_ERROR_OUT_OF_MEMORY:
        return "out of memory";
    case TUNA_ERROR_TOO_MANY_HANDLES:
        return "too many handles";
    case TUNA_ERROR_NAME_TOO_LONG:
        return "name too long";
    case TUNA_ERROR_DUPLICATE_NAME:
        return "duplicate name";
    case TUNA_ERROR_INVALID_NAME:
        return "invalid name";
    case TUNA_ERROR_MTU_TOO_SMALL:
        return "mtu too small";
    case TUNA_ERROR_MTU_TOO_BIG:
        return "mtu too big";
    case TUNA_ERROR_UNKNOWN_ADDRESS_FAMILY:
        return "unknown address family";
    default:
        return "unknown";
    }
}

