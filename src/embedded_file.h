#ifndef EMBEDDED_FILE_PRIV_INCLUDED
#define EMBEDDED_FILE_PRIV_INCLUDED

#include <stddef.h>

typedef struct {
    char const *name;
    size_t size;
    unsigned char const *data;
} embedded_file;

#endif
