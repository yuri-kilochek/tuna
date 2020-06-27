#ifndef TUNA_IMPL_INCL_ADMIN_STATE_H
#define TUNA_IMPL_INCL_ADMIN_STATE_H

#include <stdint.h>

#include <tuna/impl/prolog.inc>
///////////////////////////////////////////////////////////////////////////////

typedef uint_least8_t tuna_admin_state;

#define TUNA_ADMIN_STATE_DISABLED UINT8_C(1)
#define TUNA_ADMIN_STATE_ENABLED  UINT8_C(2)

///////////////////////////////////////////////////////////////////////////////
#include <tuna/impl/epilog.inc>

#endif

