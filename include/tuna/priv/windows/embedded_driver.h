#ifndef TUNA_EMBEDDED_DRIVER_INCLUDED
#define TUNA_EMBEDDED_DRIVER_INCLUDED

#include <tuna.h>
#include <tuna/priv/embedded_file.h>

typedef struct {
    tuna_embedded_file const *inf;
    tuna_embedded_file const *cat;
    tuna_embedded_file const *sys;
} tuna_embedded_driver;

tuna_error
tuna_install_embedded_driver(tuna_embedded_driver const *driver);

#endif
