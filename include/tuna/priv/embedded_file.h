#ifndef TUNA_EMBEDDED_FILE_INCLUDED
#define TUNA_EMBEDDED_FILE_INCLUDED

#include <stddef.h>

typedef struct {
    char const *name;
    size_t size;
    unsigned char const *data;
} tuna_embedded_file;

#endif
