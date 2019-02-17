#include <tuna.h>

///////////////////////////////////////////////////////////////////////////////

char const*
tuna_get_error_name(tuna_error err) {
    switch (err) {
      #define TUNA_PRIV_DEFINE_ERROR_NAME(value, name) \
        case value:; \
          return name; \
      /**/

      TUNA_PRIV_DEFINE_ERROR_NAME(0                    , "none"            )
      TUNA_PRIV_DEFINE_ERROR_NAME(TUNA_UNEXPECTED      , "unexpected"      )
      TUNA_PRIV_DEFINE_ERROR_NAME(TUNA_DEVICE_LOST     , "device lost"     )
      TUNA_PRIV_DEFINE_ERROR_NAME(TUNA_FORBIDDEN       , "forbidden"       )
      TUNA_PRIV_DEFINE_ERROR_NAME(TUNA_OUT_OF_MEMORY   , "out of memory"   )
      TUNA_PRIV_DEFINE_ERROR_NAME(TUNA_TOO_MANY_HANDLES, "too many handles")
      TUNA_PRIV_DEFINE_ERROR_NAME(TUNA_TOO_LONG_NAME   , "too long name"   )
      TUNA_PRIV_DEFINE_ERROR_NAME(TUNA_DUPLICATE_NAME  , "duplicate name"  )
      TUNA_PRIV_DEFINE_ERROR_NAME(TUNA_INVALID_NAME    , "invalid name"    )

      #undef TUNA_PRIV_DEFINE_ERROR_NAME

      default:;
        return NULL;
    }
}
