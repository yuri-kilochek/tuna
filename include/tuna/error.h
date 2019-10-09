#ifndef TUNA_PRIV_INCLUDE_GUARD_ERROR
#define TUNA_PRIV_INCLUDE_GUARD_ERROR

#include <tuna/api.h>

#include <stdint.h>

///////////////////////////////////////////////////////////////////////////////

typedef uint_fast8_t tuna_error;

#define TUNA_UNEXPECTED       UINT8_C( 1)
#define TUNA_UNSUPPORTED      UINT8_C( 2)
#define TUNA_DEVICE_LOST      UINT8_C( 3)
#define TUNA_FORBIDDEN        UINT8_C( 4)
#define TUNA_OUT_OF_MEMORY    UINT8_C( 5)
#define TUNA_TOO_MANY_HANDLES UINT8_C( 6)
#define TUNA_DEVICE_BUSY      UINT8_C( 7)
#define TUNA_NAME_TOO_LONG    UINT8_C( 8)
#define TUNA_DUPLICATE_NAME   UINT8_C( 9)
#define TUNA_INVALID_NAME     UINT8_C(10)
#define TUNA_MTU_TOO_SMALL    UINT8_C(11)
#define TUNA_MTU_TOO_BIG      UINT8_C(12)

TUNA_PRIV_API
char const *
tuna_get_error_name(tuna_error error);

///////////////////////////////////////////////////////////////////////////////

#endif
