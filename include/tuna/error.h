#ifndef TUNA_IMPL_INCL_ERROR_H
#define TUNA_IMPL_INCL_ERROR_H

#include <stdint.h>

#include <tuna/impl/prolog.inc>
///////////////////////////////////////////////////////////////////////////////

typedef uint_least8_t tuna_error;

#define TUNA_ERROR_UNSUPPORTED            UINT8_C( 1)
#define TUNA_ERROR_INTERFACE_NOT_FOUND    UINT8_C( 2)
#define TUNA_ERROR_INTERFACE_LOST         UINT8_C( 3)
#define TUNA_ERROR_INTERFACE_BUSY         UINT8_C( 4)
#define TUNA_ERROR_FORBIDDEN              UINT8_C( 5)
#define TUNA_ERROR_OUT_OF_MEMORY          UINT8_C( 6)
#define TUNA_ERROR_TOO_MANY_HANDLES       UINT8_C( 7)
#define TUNA_ERROR_NAME_TOO_LONG          UINT8_C( 8)
#define TUNA_ERROR_DUPLICATE_NAME         UINT8_C( 9)
#define TUNA_ERROR_INVALID_NAME           UINT8_C(10)
#define TUNA_ERROR_MTU_TOO_SMALL          UINT8_C(11)
#define TUNA_ERROR_MTU_TOO_BIG            UINT8_C(12)
#define TUNA_ERROR_UNKNOWN_ADDRESS_FAMILY UINT8_C(13)

#define TUNA_IMPL_ERROR_UNKNOWN UINT_LEAST8_MAX

TUNA_IMPL_API
char const *
tuna_get_error_name(tuna_error error);

///////////////////////////////////////////////////////////////////////////////
#include <tuna/impl/epilog.inc>

#endif

