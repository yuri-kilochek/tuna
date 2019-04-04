#ifndef TUNA_PRIV_EMBEDDED_DRIVER_INCLUDED
#define TUNA_PRIV_EMBEDDED_DRIVER_INCLUDED

#include <tuna/priv/embedded_file.h>

typedef struct {
    tuna_embedded_file const *inf;
    tuna_embedded_file const *cat;
    tuna_embedded_file const *sys;
} tuna_embedded_driver;

int tuna_install_embedded_driver(embedded_driver const *driver, char const *id);
void tuna_uninstall_embedded_driver(embedded_driver const *driver, char const *id);

#endif
