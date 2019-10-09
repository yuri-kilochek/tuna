#ifndef TUNA_PRIV_INCLUDE_GUARD
#define TUNA_PRIV_INCLUDE_GUARD

#include <tuna/api.h>
#include <tuna/os.h>
#include <tuna/version.h>
#include <tuna/error.h>

#include <stddef.h>

///////////////////////////////////////////////////////////////////////////////

typedef struct tuna_device_ref tuna_device_ref;

TUNA_PRIV_API
tuna_error
tuna_allocate(tuna_device_ref **ref);

TUNA_PRIV_API
void
tuna_free(tuna_device_ref *ref);

///////////////////////////////////////////////////////////////////////////////

TUNA_PRIV_API
tuna_error
tuna_get_index(tuna_device_ref const *ref, unsigned int *index);

///////////////////////////////////////////////////////////////////////////////

TUNA_PRIV_API
tuna_error
tuna_set_name(tuna_device_ref *ref, char const *name);

TUNA_PRIV_API
tuna_error
tuna_get_name(tuna_device_ref const *ref, char **name);

TUNA_PRIV_API
tuna_error
tuna_free_name(char *name);

///////////////////////////////////////////////////////////////////////////////

typedef uint_fast8_t tuna_ownership;
#define TUNA_EXCLUSIVE UINT8_C(0)
#define TUNA_SHARED    UINT8_C(1)

TUNA_PRIV_API
tuna_error
tuna_get_ownership(tuna_device_ref const *ref, tuna_ownership *ownership);

///////////////////////////////////////////////////////////////////////////////

typedef uint_fast8_t tuna_lifetime;
#define TUNA_TRANSIENT  UINT8_C(0)
#define TUNA_PERSISTENT UINT8_C(1)

TUNA_PRIV_API
tuna_error
tuna_set_lifetime(tuna_device_ref *ref, tuna_lifetime lifetime);

TUNA_PRIV_API
tuna_error
tuna_get_lifetime(tuna_device_ref const *ref, tuna_lifetime *lifetime);

///////////////////////////////////////////////////////////////////////////////

typedef uint_fast8_t tuna_link_state;
#define TUNA_DISCONNECTED UINT8_C(0)
#define TUNA_CONNECTED    UINT8_C(1)

TUNA_PRIV_API
tuna_error
tuna_set_link_state(tuna_device_ref *ref, tuna_link_state link_state);

TUNA_PRIV_API
tuna_error
tuna_get_link_state(tuna_device_ref const *ref, tuna_link_state *link_state);

///////////////////////////////////////////////////////////////////////////////

TUNA_PRIV_API
tuna_error
tuna_set_mtu(tuna_device *device, size_t mtu);

TUNA_PRIV_API
tuna_error
tuna_get_mtu(tuna_device const *device, size_t *mtu);

///////////////////////////////////////////////////////////////////////////////

typedef uint_least8_t tuna_address_family;
#define TUNA_IP4 UINT8_C(1)
#define TUNA_IP6 UINT8_C(2)

typedef struct tuna_address {
    tuna_address_family family;
} tuna_address;

typedef struct tuna_ip4_address {
    tuna_address_family family;
    uint_least8_t value[4];
    uint_least8_t prefix_length;
} tuna_ip4_address;

typedef struct tuna_ip6_address {
    tuna_address_family family;
    uint_least8_t value[16];
    uint_least8_t prefix_length;
} tuna_ip6_address;

TUNA_PRIV_API
tuna_error
tuna_add_address(tuna_device *device, tuna_address const *address);

TUNA_PRIV_API
tuna_error
tuna_remove_address(tuna_device *device, tuna_address const *address);

typedef struct tuna_address_list tuna_address_list;

TUNA_PRIV_API
void
tuna_free_address_list(tuna_address_list *list);

TUNA_PRIV_API
size_t
tuna_get_address_count(tuna_address_list const *list);

TUNA_PRIV_API
tuna_address const *
tuna_get_address_at(tuna_address_list const *list, size_t index);

TUNA_PRIV_API
tuna_error
tuna_get_address_list(tuna_device const *device, tuna_address_list **list);

///////////////////////////////////////////////////////////////////////////////

#if TUNA_PRIV_OS_WINDOWS
    typedef void *tuna_io_handle;
#else
    typedef int tuna_io_handle;
#endif

TUNA_PRIV_API
tuna_io_handle
tuna_get_io_handle(tuna_device_ref *ref);

///////////////////////////////////////////////////////////////////////////////

typedef struct tuna_device_list tuna_device_list;

TUNA_PRIV_API
void
tuna_free_device_list(tuna_device_list *list);

TUNA_PRIV_API
size_t
tuna_get_device_count(tuna_device_list const *list);

TUNA_PRIV_API
tuna_device const *
tuna_get_device_at(tuna_device_list const *list, size_t index);

TUNA_PRIV_API
tuna_error
tuna_get_device_list(tuna_device_list **list);

///////////////////////////////////////////////////////////////////////////////

TUNA_PRIV_API
tuna_error
tuna_bind_as(tuna_device_ref *ref, tuna_device_ref const *example_ref);

TUNA_PRIV_API
tuna_error
tuna_bind_by_index(tuna_device_ref *ref, unsigned int index);

TUNA_PRIV_API
tuna_error
tuna_bind_by_name(tuna_device_ref *ref, char const *name);

TUNA_PRIV_API
int
tuna_is_bound(tuna_device_ref const *ref);

TUNA_PRIV_API
void
tuna_unbind(tuna_device_ref *ref);

///////////////////////////////////////////////////////////////////////////////

TUNA_PRIV_API
tuna_error
tuna_create(tuna_device_ref *ref, tuna_ownership ownership);

TUNA_PRIV_API
tuna_error
tuna_open(tuna_device_ref *ref);

TUNA_PRIV_API
int
tuna_is_open(tuna_device_ref const *ref);

TUNA_PRIV_API
void
tuna_close(tuna_device_ref *ref);

///////////////////////////////////////////////////////////////////////////////
#if __cplusplus
} // extern "C"
#endif
///////////////////////////////////////////////////////////////////////////////

#endif
