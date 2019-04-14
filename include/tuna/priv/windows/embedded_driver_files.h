#ifndef TUNA_EMBEDDED_DRIVER_FILES_INCLUDED
#define TUNA_EMBEDDED_DRIVER_FILES_INCLUDED

#include <tuna/priv/embedded_file.h>

typedef struct {
    tuna_embedded_file const *inf;
    tuna_embedded_file const *cat;
    tuna_embedded_file const *sys;
} tuna_embedded_driver_files;

#endif
