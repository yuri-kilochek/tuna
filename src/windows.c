#include <tuna.h>

#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <windows.h>
#include <shlwapi.h>
#include <pathcch.h>
#include <cfgmgr32.h>
#include <setupapi.h>
#include <newdev.h>

#include <tap-windows.h>

#include <tuna/priv/windows/embedded_driver.h>

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
    wchar_t *path = NULL;
    HANDLE handle = INVALID_HANDLE_VALUE;

    tuna_error err = 0;

    if ((err = tuna_join_paths(&path, dir, file->name))) { goto out; }
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

    return err;
}

typedef struct {
    wchar_t *dir;
    wchar_t *inf_path;
    wchar_t *cat_path;
    wchar_t *sys_path;
} tuna_extruded_driver;

static
void
tuna_erase_extruded_driver(tuna_extruded_driver const *driver) {
    tuna_erase_extruded_file(driver->sys_path);
    tuna_erase_extruded_file(driver->cat_path);
    tuna_erase_extruded_file(driver->inf_path);

    if (driver->dir) { RemoveDirectoryW(driver->dir); }
    free(driver->dir);
}

static
tuna_error
tuna_extrude_driver(tuna_extruded_driver *extruded_out,
                    tuna_embedded_driver const *embedded)
{
    tuna_extruded_driver extruded = {0};

    tuna_error err = 0;

    wchar_t temp_dir[MAX_PATH + 1];
    if (!GetTempPathW(sizeof(temp_dir) / sizeof(*temp_dir), temp_dir)) {
        err = tuna_translate_error(GetLastError());
        goto out;
    }
    wchar_t dir_name[/*max path component length:*/256];
    if (swprintf(dir_name, sizeof(dir_name) / sizeof(*dir_name),
                 L"TunaDriver-%lu", (unsigned long)GetCurrentThreadId()) < 0)
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

    if ((err = tuna_extrude_file(&extruded.inf_path,
                                 extruded.dir,
                                 embedded->inf_file))) { goto out; }
    if ((err = tuna_extrude_file(&extruded.cat_path,
                                 extruded.dir,
                                 embedded->cat_file))) { goto out; }
    if ((err = tuna_extrude_file(&extruded.sys_path,
                                 extruded.dir,
                                 embedded->sys_file))) { goto out; }

    *extruded_out = extruded;

  out:;
    if (err) { tuna_erase_extruded_driver(&extruded); }

    return err;
}

static
tuna_error
tuna_install_driver(tuna_embedded_driver const *embedded,
                    wchar_t const *hardware_id)
{
    tuna_extruded_driver extruded = {0};
    HDEVINFO dev_info = INVALID_HANDLE_VALUE;
    _Bool registered = 0;
    
    tuna_error err = 0;
    
    if ((err = tuna_extrude_driver(&extruded, embedded))) { goto out; }

    GUID class_guid;
    wchar_t class_name[MAX_CLASS_NAME_LEN];
    if (!SetupDiGetINFClassW(extruded.inf_path,
                             &class_guid,
                             class_name,
                             sizeof(class_name) / sizeof(*class_name),
                             NULL))
    {
        err = tuna_translate_error(GetLastError());
        goto out;
    }

    dev_info = SetupDiCreateDeviceInfoList(&class_guid, NULL);
    if (dev_info == INVALID_HANDLE_VALUE) {
        err = tuna_translate_error(GetLastError());
        goto out;
    }

    SP_DEVINFO_DATA dev_info_data = {.cbSize = sizeof(dev_info_data)};
    if (!SetupDiCreateDeviceInfoW(dev_info,
                                  class_name, &class_guid,
                                  NULL,
                                  NULL,
                                  DICD_GENERATE_ID,
                                  &dev_info_data))
    {
        err = tuna_translate_error(GetLastError());
        goto out;
    }

    assert(wcslen(hardware_id) <= MAX_DEVICE_ID_LEN);
    wchar_t hardware_ids[MAX_DEVICE_ID_LEN + 1 + 1];
    wcscpy(hardware_ids, hardware_id);
    hardware_ids[wcslen(hardware_ids) + 1] = L'\0';


    if (!SetupDiSetDeviceRegistryPropertyW(dev_info, &dev_info_data,
                                           SPDRP_HARDWAREID,
                                           (BYTE *)&hardware_ids,
                                           (DWORD)sizeof(hardware_ids)))
    {
        err = tuna_translate_error(GetLastError());
        goto out;
    }

    if (!SetupDiCallClassInstaller(DIF_REGISTERDEVICE,
                                   dev_info, &dev_info_data))
    {
        err = tuna_translate_error(GetLastError());
        goto out;
    }
    registered = 1;

    BOOL need_reboot;
    if (!UpdateDriverForPlugAndPlayDevicesW(NULL,
                                            hardware_id,
                                            extruded.inf_path,
                                            0,
                                            &need_reboot))
    {
        err = tuna_translate_error(GetLastError());
        goto out;
    }
    assert(!need_reboot);

    wchar_t new_hwids[MAX_DEVICE_ID_LEN + 1 + 1] = L"footun\0";
    if (!SetupDiSetDeviceRegistryPropertyW(dev_info, &dev_info_data,
                                           SPDRP_HARDWAREID,
                                           (BYTE *)&new_hwids,
                                           (DWORD)sizeof(new_hwids)))
    {
        err = tuna_translate_error(GetLastError());
        goto out;
    }

  out:;
    if (err && registered) {
        SetupDiCallClassInstaller(DIF_REMOVE, dev_info, &dev_info_data);
    }
    if (dev_info != INVALID_HANDLE_VALUE) {
        SetupDiDestroyDeviceInfoList(dev_info);
    }
    tuna_erase_extruded_driver(&extruded);

    return err;
}

extern tuna_embedded_driver const tuna_embedded_tap_windows;

struct tuna_device {
    HANDLE handle;
};

tuna_error
tuna_create_device(tuna_device **device) {
    tuna_device *dev = NULL;

    tuna_error err = 0;

    if ((err = tuna_install_driver(&tuna_embedded_tap_windows,
                                   TUNA_TAP_WINDOWS_HARDWARE_ID)))
    { goto fail; }

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
