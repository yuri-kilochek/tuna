#include <tuna.h>
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

struct tuna_device {
    HANDLE hFile;
};


//
//tuna_error
//tuna_create_device(tuna_device **ppDevice) {
//    tuna_device *pDevice = NULL;
//    //HKEY hkAdapter = 0;
//
//    //DWORD dwErrCode;
//    tuna_error err = 0;
//
//    if (!(pDevice = malloc(sizeof(*pDevice)))) {
//        err = TUNA_OUT_OF_MEMORY;
//        goto fail;
//    }
//    *pDevice = (tuna_device){.hFile = INVALID_HANDLE_VALUE};
//
//    //if ((dwErrCode = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
//    //                               ADAPTER_KEY,
//    //                               0,
//    //                               KEY_READ,
//    //                               &hkAdapter)))
//    //{
//    //    hkAdapter = 0;
//    //    err = TunaPrivTranslateErrCode(dwErrCode);
//    //    goto fail;
//    //}
//
//    //char szSubkey[256];
//    //strcpy(szSubkey, ADAPTER_KEY);
//    //strcat(szSubkey, "\\");
//    //DWORD dwSubkeyLen = (DWORD)strlen(szSubkey);
//    //for (DWORD i = 0; ; ++i)
//    //{
//    //    dwErrCode = RegEnumKeyExA(hkAdapter,
//    //                              i,
//    //                              szSubkey + dwSubkeyLen,
//    //                              &(DWORD){sizeof(szSubkey) - dwSubkeyLen},
//    //                              NULL,
//    //                              NULL,
//    //                              NULL,
//    //                              NULL);
//    //    if (dwErrCode)
//    //    {
//    //        if (dwErrCode == ERROR_NO_MORE_ITEMS) break;
//    //        err = TunaPrivTranslateErrCode(dwErrCode);
//    //        goto fail;
//    //    }
//
//
//
//    //}
//
//
//
//
//
//
//    *ppDevice = pDevice;
//
//  done:;
//    //if (hkAdapter) { RegCloseKey(hkAdapter); }
//    return err;
//  fail:;
//    tuna_destroy_device(pDevice);
//    goto done;
//}
//
//void
//tuna_destroy_device(tuna_device *pDevice)
//{
//    if (pDevice) {
//        if (pDevice->hFile != INVALID_HANDLE_VALUE) {
//            CloseHandle(pDevice->hFile);
//        }
//    }
//    free(pDevice);
//}
