#ifndef TUNA_IMPL_INCL_ADDRESS_H
#define TUNA_IMPL_INCL_ADDRESS_H

#include <stdint.h>

#include <tuna/impl/prolog.inc>
///////////////////////////////////////////////////////////////////////////////

typedef uint_least8_t tuna_address_family;

#define TUNA_ADDRESS_FAMILY_IP4 UINT8_C(1)
#define TUNA_ADDRESS_FAMILY_IP6 UINT8_C(2)

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

///////////////////////////////////////////////////////////////////////////////
#include <tuna/impl/epilog.inc>

#endif

