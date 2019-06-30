#ifndef TUNA_WINDOWS_EMBEDDED_DRIVER_INCLUDED
#define TUNA_WINDOWS_EMBEDDED_DRIVER_INCLUDED

#include <tuna/priv/windows/embedded_file.h>

typedef struct {
    wchar_t const *hardware_id;
    tuna_embedded_file const *inf_file;
    tuna_embedded_file const *cat_file;
    tuna_embedded_file const *sys_file;
} tuna_embedded_driver;

#endif
