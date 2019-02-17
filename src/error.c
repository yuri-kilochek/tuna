#include <tuna.h>

///////////////////////////////////////////////////////////////////////////////

char const*
tuna_get_error_name(tuna_error err) {
    switch (err) {
      case 0:;
        return "none";
      case TUNA_UNEXPECTED:;
        return "unexpected";
      case TUNA_DEVICE_LOST:;
        return "device lost";
      case TUNA_FORBIDDEN:;
        return "forbidden";
      case TUNA_OUT_OF_MEMORY:;
        return "out of memory";
      case TUNA_TOO_MANY_HANDLES:;
        return "too many handles";
      case TUNA_NAME_TOO_LONG:;
        return "name too long";
      case TUNA_DUPLICATE_NAME:;
        return "duplicate name";
      case TUNA_INVALID_NAME:;
        return "invalid name";
      case TUNA_MTU_TOO_SMALL:;
        return "mtu too small";
      case TUNA_MTU_TOO_BIG:;
        return "mtu too big";
      default:;
        return NULL;
    }
}
