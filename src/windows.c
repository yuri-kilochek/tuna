#include <tuna.h>
#include <tuna/priv/windows/translate_err_code.h>
#include <tuna/priv/windows/embedded_driver.h>

#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <windows.h>

#include <tap-windows.h>

///////////////////////////////////////////////////////////////////////////////

#pragma warning(disable: 4996) // No, MSVC, C functions are not deprecated.

extern
tuna_embedded_driver const tuna_tap_windows;

struct tuna_device {
    HANDLE handle;
};

tuna_error
tuna_create_device(tuna_device **device) {
    tuna_device *dev = NULL;

    tuna_error err = 0;

    if ((err = tuna_install_embedded_driver(&tuna_tap_windows))) { goto fail; }

    if (!(dev = malloc(sizeof(*dev)))) {
        err = TUNA_OUT_OF_MEMORY;
        goto fail;
    }
    *dev = (tuna_device){.handle = INVALID_HANDLE_VALUE};





    *device = dev;

  done:;
    return err;
  fail:;
    tuna_destroy_device(dev);
    goto done;
}

void
tuna_destroy_device(tuna_device *dev)
{
    if (dev) {
        if (dev->handle != INVALID_HANDLE_VALUE) {
            CloseHandle(dev->handle);
        }
    }
    free(dev);
}
