#include <tuna.h>

///////////////////////////////////////////////////////////////////////////////

tuna_version_t
tuna_get_actual_version(void) {
    return TUNA_HEADER_VERSION;
}

///////////////////////////////////////////////////////////////////////////////

char const*
tuna_get_error_name(tuna_error_t error) {
    switch (error) {
      case 0:;
        return "0";

      #define TUNA_PRIV_DEFINE_ERROR_NAME(upper_name, lower_name) \
        case TUNA_##upper_name:; \
          return #lower_name; \
      /**/
      TUNA_PRIV_ENUMERATE_ERRORS(TUNA_PRIV_DEFINE_ERROR_NAME)
      #undef TUNA_PRIV_DEFINE_ERROR_NAME

      default:;
        return NULL;
    }
}
