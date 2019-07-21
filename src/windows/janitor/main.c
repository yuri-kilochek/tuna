#include <stdio.h>

#include <windows.h>

int main(void) {
    wchar_t **args = NULL;

    DWORD err_code = ERROR_SUCCESS;

    int arg_count;
    if (!(args = CommandLineToArgvW(GetCommandLineW(), &arg_count))) {
        err_code = GetLastError();
        goto out;
    }

    for (int i = 0; i < arg_count; ++i) {
        fwprintf(stderr, L"----- [%d]: %s\n", i, args[i]);
    }

out:
    LocalFree(args);

    return err_code;
}
