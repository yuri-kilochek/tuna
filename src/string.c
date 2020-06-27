#include <tuna/priv/string.h>
#include <tuna/string.h>

#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////

tuna_error
tuna_allocate_string(size_t length, char **string_out) {
    char *string = NULL;

    tuna_error error = 0;

    if (!(string = malloc(length + 1))) {
        error = TUNA_ERROR_OUT_OF_MEMORY;
        goto out;
    }

    *string_out = string; string = NULL;
out:
    tuna_free_string(string);

    return error;
}

void
tuna_free_string(char *string) {
    free(string);
}

