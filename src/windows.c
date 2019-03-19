#include <tuna.h>

#include <stddef.h>

#include <windows.h>

#include <tap-windows.h>

///////////////////////////////////////////////////////////////////////////////

extern
char const *const tap_windows_inf_name;
extern
size_t const tap_windows_inf_size;
extern
unsigned char const *const tap_windows_inf_data;

extern
char const *const tap_windows_cat_name;
extern
size_t const tap_windows_cat_size;
extern
unsigned char const *const tap_windows_cat_data;

extern
char const *const tap_windows_sys_name;
extern
size_t const tap_windows_sys_size;
extern
unsigned char const *const tap_windows_sys_data;

tuna_error
tuna_create_device(tuna_device **lpDevice) {
    return 0;
}

void
tuna_destroy_device(tuna_device *lpDevice) {

}


