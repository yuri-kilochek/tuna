#include <tuna.h>

#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>

#include <windows.h>
#include <shlwapi.h>
#include <pathcch.h>
#include <cfgmgr32.h>
#include <setupapi.h>
#include <newdev.h>

#include <tap-windows.h>

#include <tuna/priv/windows/embedded_driver_files.h>

///////////////////////////////////////////////////////////////////////////////

#pragma warning(disable: 4996) // No, MSVC, C functions are not deprecated.

static
tuna_error
tuna_translate_error(DWORD err_code) {
    switch (err_code) {
      case 0:;
        return 0;
      default:;
        return TUNA_UNEXPECTED;
    }
}

static
tuna_error
tuna_translate_hresult(HRESULT hres) {
    switch (hres) {
      case 0:;
        return 0;
      default:;
        return TUNA_UNEXPECTED;
    }
}

static
tuna_error
tuna_utf8to16(wchar_t **utf16_out, char const *utf8) {
    wchar_t *utf16 = NULL;

    tuna_error err = 0;

    int utf16_size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                         utf8, -1,
                                         NULL, 0);
    if (utf16_size == 0) {
        err = tuna_translate_error(GetLastError());
        goto out;
    }

    if (!(utf16 = malloc(utf16_size * sizeof(*utf16)))) {
        err = TUNA_OUT_OF_MEMORY;
        goto out;
    }

    int new_utf16_size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                             utf8, -1,
                                             utf16, utf16_size);
    if (new_utf16_size == 0) {
        err = tuna_translate_error(GetLastError());
        goto out;
    }
    if (new_utf16_size != utf16_size) {
        err = TUNA_UNEXPECTED;
        goto out;
    }

    *utf16_out = utf16;
    
  out:;
    if (err) { free(utf16); }

    return err;
}

static
tuna_error
tuna_join_paths(wchar_t **joined_out,
                wchar_t const *base, wchar_t const *extra)
{
    wchar_t *local_joined = NULL;
    wchar_t *joined = NULL;

    tuna_error err = 0;

    HRESULT hres = PathAllocCombine(base, extra, PATHCCH_ALLOW_LONG_PATHS,
                                    &local_joined);
    if (hres) {
        local_joined = NULL;
        err = tuna_translate_hresult(hres);
        goto out;
    }

    if (!(joined = malloc((wcslen(local_joined) + 1) * sizeof(*joined)))) {
        err = TUNA_OUT_OF_MEMORY;
        goto out;
    }
    wcscpy(joined, local_joined);

    *joined_out = joined;

  out:;
    if (err) { free(joined); }
    LocalFree(local_joined);

    return err;
}

static
void
tuna_erase_extruded_file(wchar_t *path) {
    if (path) { DeleteFileW(path); }
    free(path);
}

static
tuna_error
tuna_extrude_file(wchar_t **path_out,
                  wchar_t const *dir,
                  tuna_embedded_file const *file)
{
    wchar_t *name = NULL;
    wchar_t *path = NULL;
    HANDLE handle = INVALID_HANDLE_VALUE;

    tuna_error err = 0;

    if ((err = tuna_utf8to16(&name, file->name))) { goto out; }
    if ((err = tuna_join_paths(&path, dir, name))) { goto out; }
    handle = CreateFileW(path,
                         GENERIC_WRITE,
                         0,
                         NULL,
                         CREATE_ALWAYS,
                         FILE_ATTRIBUTE_TEMPORARY,
                         NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        err = tuna_translate_error(GetLastError());
        goto out;
    }

    if (!WriteFile(handle,
                   file->data, (DWORD)file->size,
                   &(DWORD){0},
                   NULL))
    {
        err = tuna_translate_error(GetLastError());
        goto out;
    }

    *path_out = path;

  out:;
    if (handle != INVALID_HANDLE_VALUE) { CloseHandle(handle); }
    if (err) { tuna_erase_extruded_file(path); }
    free(name);

    return err;
}

typedef struct {
    wchar_t *dir;
    wchar_t *inf;
    wchar_t *cat;
    wchar_t *sys;
} tuna_extruded_driver_files;

static
void
tuna_erase_extruded_driver_files(tuna_extruded_driver_files const *files) {
    tuna_erase_extruded_file(files->sys);
    tuna_erase_extruded_file(files->cat);
    tuna_erase_extruded_file(files->inf);

    if (files->dir) { RemoveDirectoryW(files->dir); }
    free(files->dir);
}

static
tuna_error
tuna_extrude_driver_files(tuna_extruded_driver_files *extruded_out,
                          tuna_embedded_driver_files const *embedded)
{
    tuna_extruded_driver_files extruded = {0};

    tuna_error err = 0;

    wchar_t temp_dir[MAX_PATH + 1];
    if (!GetTempPathW(sizeof(temp_dir) / sizeof(*temp_dir), temp_dir)) {
        err = tuna_translate_error(GetLastError());
        goto out;
    }
    wchar_t dir_name[/*max path component length:*/256];
    if (swprintf(dir_name, sizeof(dir_name) / sizeof(*dir_name),
                 L"TunaDriverFiles-%u", (unsigned)GetCurrentThreadId()) < 0)
    {
        err = TUNA_UNEXPECTED;
        goto out;
    }
    if ((err = tuna_join_paths(&extruded.dir, temp_dir, dir_name))) {
        goto out;
    }
    if (!CreateDirectoryW(extruded.dir, NULL)) {
        DWORD err_code = GetLastError();
        if (err_code != ERROR_ALREADY_EXISTS) {
            err = tuna_translate_error(err_code);
            goto out;
        }
    }

    if ((err = tuna_extrude_file(&extruded.inf,
                                 extruded.dir,
                                 embedded->inf))) { goto out; }
    if ((err = tuna_extrude_file(&extruded.cat,
                                 extruded.dir,
                                 embedded->cat))) { goto out; }
    if ((err = tuna_extrude_file(&extruded.sys,
                                 extruded.dir,
                                 embedded->sys))) { goto out; }

    *extruded_out = extruded;

  out:;
    if (err) { tuna_erase_extruded_driver_files(&extruded); }

    return err;
}

//static
//tuna_error
//tuna_install_embedded_driver(tuna_embedded_driver_files const *embedded_driver_files) {
//    int extruded = 0;
//    HDEVINFO dev_info = INVALID_HANDLE_VALUE;
//    int registered = 0;
//    
//    tuna_error err = 0;
//    
//    tuna_extruded_driver extruded_driver;
//    if ((err = tuna_extrude_driver(&extruded_driver, embedded_driver_files))) {
//        goto out;
//    }
//    extruded = 1;
//
//    GUID class_guid;
//    char class_name[MAX_CLASS_NAME_LEN];
//    if (!SetupDiGetINFClassA(extruded_driver.inf,
//                             &class_guid,
//                             class_name,
//                             sizeof(class_name),
//                             NULL))
//    {
//        err = tuna_translate_err_code(GetLastError());
//        goto out;
//    }
//
//    dev_info = SetupDiCreateDeviceInfoList(&class_guid, NULL);
//    if (dev_info == INVALID_HANDLE_VALUE) {
//        err = tuna_translate_err_code(GetLastError());
//        goto out;
//    }
//
//    SP_DEVINFO_DATA dev_info_data = {.cbSize = sizeof(dev_info_data)};
//    if (!SetupDiCreateDeviceInfoA(dev_info,
//                                  class_name, &class_guid,
//                                  NULL,
//                                  NULL,
//                                  DICD_GENERATE_ID,
//                                  &dev_info_data))
//    {
//        err = tuna_translate_err_code(GetLastError());
//        goto out;
//    }
//
//    char hardware_id[MAX_PATH]; {
//        char const *sys_name = embedded_driver_files->sys->name;
//        size_t len = PathFindExtensionA(sys_name) - sys_name;
//        memcpy(hardware_id, sys_name, len);
//        memset(hardware_id + len, 0, 2);
//    }
//
//    if (!SetupDiSetDeviceRegistryProperty(dev_info, &dev_info_data,
//                                          SPDRP_HARDWAREID,
//                                          (BYTE const *)&hardware_id,
//                                          (DWORD)(strlen(hardware_id) + 2)))
//    {
//        err = tuna_translate_err_code(GetLastError());
//        goto out;
//    }
//
//    if (!SetupDiCallClassInstaller(DIF_REGISTERDEVICE,
//                                   dev_info, &dev_info_data))
//    {
//        err = tuna_translate_err_code(GetLastError());
//        goto out;
//    }
//    registered = 1;
//
//    BOOL need_reboot;
//    if (!UpdateDriverForPlugAndPlayDevicesA(NULL,
//                                            hardware_id,
//                                            extruded_driver.inf,
//                                            0,
//                                            &need_reboot))
//    {
//        err = tuna_translate_err_code(GetLastError());
//        goto out;
//    }
//    if (need_reboot) {
//        err = TUNA_UNEXPECTED;
//        goto out;
//    }
//
//    char const new_hardware_id[] = "footun\0";
//    if (!SetupDiSetDeviceRegistryProperty(dev_info, &dev_info_data,
//                                          SPDRP_HARDWAREID,
//                                          (BYTE const *)&new_hardware_id,
//                                          sizeof(new_hardware_id)))
//    {
//        err = tuna_translate_err_code(GetLastError());
//        goto out;
//    }
//
//  out:;
//    if (err && registered) {
//        SetupDiCallClassInstaller(DIF_REMOVE, dev_info, &dev_info_data);
//    }
//    if (dev_info != INVALID_HANDLE_VALUE) {
//        SetupDiDestroyDeviceInfoList(dev_info);
//    }
//    if (extruded) { tuna_erase_extruded_driver(&extruded_driver); }
//    return err;
//}

extern
tuna_embedded_driver_files const tuna_embedded_tap_windows_files;

struct tuna_device {
    HANDLE handle;
};

tuna_error
tuna_create_device(tuna_device **device) {
    tuna_device *dev = NULL;

    tuna_error err = 0;

    //if ((err = tuna_install_embedded_driver(&tuna_embedded_tap_windows_files))) { goto fail; }

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
