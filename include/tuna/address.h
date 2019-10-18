#ifndef TUNA_PRIV_INCLUDE_GUARD_ADDRESS
#define TUNA_PRIV_INCLUDE_GUARD_ADDRESS

#include <tuna/api.h>
#include <tuna/error.h>
#include <tuna/ref.h>

#include <stdint.h>

///////////////////////////////////////////////////////////////////////////////

typedef uint_least8_t tuna_address_family;
#define TUNA_IP4 UINT8_C(1)
#define TUNA_IP6 UINT8_C(2)

typedef struct tuna_ip4_address {
    tuna_address_family family;
    uint_least8_t bytes[4];
    uint_least8_t prefix_length;
} tuna_ip4_address;

typedef struct tuna_ip6_address {
    tuna_address_family family;
    uint_least8_t bytes[16];
    uint_least8_t prefix_length;
} tuna_ip6_address;

typedef union tuna_address {
    tuna_address_family family;
    tuna_ip4_address ip4;
    tuna_ip6_address ip6;
} tuna_address;

TUNA_PRIV_API
tuna_error
tuna_add_address(tuna_ref *ref, tuna_address address);

TUNA_PRIV_API
tuna_error
tuna_remove_address(tuna_ref *ref, tuna_address address);

///////////////////////////////////////////////////////////////////////////////

#endif
