#include <tuna.h>

#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <windows.h>
#include <shlwapi.h>

#include <tap-windows.h>
#include <embedded-tap-windows-files.h>

///////////////////////////////////////////////////////////////////////////////

#pragma warning(disable: 4996) // No, MSVC, C functions are not deprecated.

struct tuna_device {
    HANDLE hFile;
};

static
tuna_error
tuna_priv_translate_errcode(DWORD errcode) {
    switch (errcode) {
      case 0:;
        return 0;
      default:;
        return TUNA_UNEXPECTED;
    }
}

static
tuna_error
tuna_priv_get_driver_dir_path(char *path, size_t max_path_size)
{
    if (!GetTempPathA((DWORD)max_path_size, path)) {
        return tuna_priv_translate_errcode(GetLastError());
    }
    size_t tmp_path_len = strlen(path);
    path += tmp_path_len;
    max_path_size -= tmp_path_len;

    if ((unsigned)snprintf(path, max_path_size,
                           "TunaDriverFiles%"PRIu32,
                           (uint32_t)GetCurrentThreadId()) >= max_path_size)
    { return TUNA_UNEXPECTED; }

    return 0;
}

static
tuna_error
tuna_priv_emit_tap_windows_file(char const *dir_path,
                                embedded_tap_windows_file const *embedded_file)
{
    HANDLE file_handle = INVALID_HANDLE_VALUE;

    tuna_error err = 0;

    char file_path[MAX_PATH];
    if (!PathCombineA(file_path, dir_path, embedded_file->name)) {
        err = TUNA_UNEXPECTED;
        goto fail;
    }

    if ((file_handle = CreateFile(file_path,
                                  GENERIC_WRITE,
                                  0,
                                  NULL,
                                  CREATE_ALWAYS,
                                  FILE_ATTRIBUTE_TEMPORARY,
                                  NULL)) == INVALID_HANDLE_VALUE)
    {
        err = tuna_priv_translate_errcode(GetLastError());
        goto fail;
    }

    if (!WriteFile(file_handle,
                   embedded_file->data,
                   (DWORD)embedded_file->size,
                   &(DWORD){0},
                   NULL))
    {
        err = tuna_priv_translate_errcode(GetLastError());
        goto fail;
    }

  done:;
    if (file_handle != INVALID_HANDLE_VALUE) { CloseHandle(file_handle); }
    return err;
  fail:;
    goto done;
}


//static
//tuna_error
//tuna_priv_emit_tap_windows_files(char *dir_path_)
//{
//    char dir_path[MAX_PATH];
//
//    tuna_error err = 0;
//
//
//    char szFile[MAX_PATH];
//    if (!PathCombineA(szFile, pszDirectory, pEmbeddedFile->pszName)) {
//        err = TUNA_UNEXPECTED;
//        goto fail;
//    }
//
//
//  done:;
//    return err;
//  fail:;
//    goto done;
//}
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
