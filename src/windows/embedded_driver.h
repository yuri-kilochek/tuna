#ifndef EMBEDDED_DRIVER_PRIV_INCLUDED
#define EMBEDDED_DRIVER_PRIV_INCLUDED

#include <embedded_file.h>

typedef struct {
    embedded_file const *inf;
    embedded_file const *cat;
    embedded_file const *sys;
} embedded_driver;

int install_embedded_driver(embedded_driver const *driver, char const *id);
void uninstall_embedded_driver(embedded_driver const *driver, char const *id);

#endif
