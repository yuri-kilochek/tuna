#ifndef TUNA_IMPL_INCL_LINK_STATE_H
#define TUNA_IMPL_INCL_LINK_STATE_H

#include <stdint.h>

#include <tuna/impl/prolog.inc>
///////////////////////////////////////////////////////////////////////////////

typedef uint_least8_t tuna_link_state;

#define TUNA_LINK_STATE_DISCONNECTED UINT8_C(1)
#define TUNA_LINK_STATE_CONNECTED    UINT8_C(2)

///////////////////////////////////////////////////////////////////////////////
#include <tuna/impl/epilog.inc>

#endif

