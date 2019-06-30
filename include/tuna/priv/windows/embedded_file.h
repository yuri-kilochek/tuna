#ifndef TUNA_WINDOWS_EMBEDDED_FILE_INCLUDED
#define TUNA_WINDOWS_EMBEDDED_FILE_INCLUDED

#include <stddef.h>

typedef struct {
    wchar_t const *name;
    size_t size;
    unsigned char const *data;
} tuna_embedded_file;

#endif
