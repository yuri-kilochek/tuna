#ifndef TUNA_WINDOWS_EMBEDDED_DRIVER_INCLUDED
#define TUNA_WINDOWS_EMBEDDED_DRIVER_INCLUDED

#include <stddef.h>

typedef struct {
    wchar_t const *name;
    size_t size;
    unsigned char const *data;
} tuna_embedded_file;

typedef struct {
    tuna_embedded_file const *inf_file;
    tuna_embedded_file const *cat_file;
    tuna_embedded_file const *sys_file;
} tuna_embedded_driver;

#endif
