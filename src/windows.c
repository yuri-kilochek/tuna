#include <tuna.h>

#include <stdlib.h>

#include <windows.h>

#include <tap-windows.h>
#include <tap-windows-embedded-files.h>

///////////////////////////////////////////////////////////////////////////////

struct tuna_device {
    HANDLE hFile;
};

static
tuna_error
TunaPrivTranslateErrCode(DWORD dwErrCode) {
    switch (dwErrCode) {
      case ERROR_SUCCESS:;
        return 0;
      default:;
        return TUNA_UNEXPECTED;
    }
}

tuna_error
tuna_create_device(tuna_device **ppDevice)
{
    HKEY hkAdapters = 0;

    DWORD dwErrCode;
    tuna_error err = 0;

    tuna_device *pDevice = malloc(sizeof(*pDevice));
    if (!pDevice) {
        err = TUNA_OUT_OF_MEMORY;
        goto fail;
    }
    *pDevice = (tuna_device){.hFile = INVALID_HANDLE_VALUE};

    if ((dwErrCode = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                                   ADAPTER_KEY,
                                   0,
                                   KEY_READ,
                                   &hkAdapters)))
    {
        hkAdapters = 0;
        err = TunaPrivTranslateErrCode(dwErrCode);
        goto fail;
    }





    *ppDevice = pDevice;

  done:;
    if (hkAdapters) { RegCloseKey(hkAdapters); }
    return err;
  fail:;
    tuna_destroy_device(pDevice);
    goto done;
}

void
tuna_destroy_device(tuna_device *pDevice)
{
    if (pDevice) {
        if (pDevice->hFile != INVALID_HANDLE_VALUE) {
            CloseHandle(pDevice->hFile);
        }
    }
    free(pDevice);
}


