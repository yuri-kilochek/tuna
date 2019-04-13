#include <tuna/priv/windows/embedded_driver.h>

#include <inttypes.h>
#include <string.h>
#include <stdio.h>

#include <windows.h>
#include <shlwapi.h>
#include <cfgmgr32.h>
#include <setupapi.h>
#include <newdev.h>

#include <tuna/priv/windows/translate_err_code.h>

///////////////////////////////////////////////////////////////////////////////

typedef struct {
    char dir[MAX_PATH];
    char inf[MAX_PATH];
    char cat[MAX_PATH];
    char sys[MAX_PATH];
} tuna_extruded_driver;

static
tuna_error
tuna_extrude_file(char *file_path,
                  char const *dir_path,
                  tuna_embedded_file const *embedded_file)
{
    HANDLE file_handle = INVALID_HANDLE_VALUE;

    tuna_error err = 0;

    if (!PathCombineA(file_path, dir_path, embedded_file->name)) {
        err = TUNA_UNEXPECTED;
        goto out;
    }

    file_handle = CreateFile(file_path,
                             GENERIC_WRITE,
                             0,
                             NULL,
                             CREATE_ALWAYS,
                             FILE_ATTRIBUTE_TEMPORARY,
                             NULL);
    if (file_handle == INVALID_HANDLE_VALUE) {
        err = tuna_translate_err_code(GetLastError());
        goto out;
    }

    if (!WriteFile(file_handle,
                   embedded_file->data,
                   (DWORD)embedded_file->size,
                   &(DWORD){0},
                   NULL))
    {
        err = tuna_translate_err_code(GetLastError());
        goto out;
    }

  out:;
    if (file_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(file_handle);
        if (err) { DeleteFileA(file_path); }
    }

    return err;
}

static
tuna_error
tuna_extrude_driver(tuna_extruded_driver *extruded_driver,
                    tuna_embedded_driver const *embedded_driver)
{
    int dir_created = 0;
    int inf_extruded = 0;
    int cat_extruded = 0;
    int sys_extruded = 0;

    tuna_error err = 0;

    if (!GetTempPathA(MAX_PATH, extruded_driver->dir)) {
        err = tuna_translate_err_code(GetLastError());
        goto out;
    }

    char dir_name[MAX_PATH];
    if ((unsigned)snprintf(dir_name, sizeof(dir_name),
                           "TunaDriverFiles-%"PRIu32,
                           (uint32_t)GetCurrentThreadId()) >= sizeof(dir_name))
    {
        err = TUNA_UNEXPECTED;
        goto out;
    }

    if (!PathAppendA(extruded_driver->dir, dir_name)) {
        err = TUNA_UNEXPECTED;
        goto out;
    }

    if (!CreateDirectoryA(extruded_driver->dir, NULL)) {
        DWORD err_code = GetLastError();
        if (err_code != ERROR_ALREADY_EXISTS) {
            err = tuna_translate_err_code(err_code);
            goto out;
        }
    }
    dir_created = 1;

    if ((err = tuna_extrude_file(extruded_driver->inf,
                                 extruded_driver->dir,
                                 embedded_driver->inf))) { goto out; }
    inf_extruded = 1;

    if ((err = tuna_extrude_file(extruded_driver->cat,
                                 extruded_driver->dir,
                                 embedded_driver->cat))) { goto out; }
    cat_extruded = 1;

    if ((err = tuna_extrude_file(extruded_driver->sys,
                                 extruded_driver->dir,
                                 embedded_driver->sys))) { goto out; }
    sys_extruded = 1;

  out:;
    if (err && sys_extruded) { DeleteFileA(extruded_driver->sys); }
    if (err && cat_extruded) { DeleteFileA(extruded_driver->cat); }
    if (err && inf_extruded) { DeleteFileA(extruded_driver->inf); }
    if (err && dir_created) { RemoveDirectoryA(extruded_driver->dir); }

    return err;
}

static
void
tuna_erase_extruded_driver(tuna_extruded_driver const *driver) {
    DeleteFileA(driver->sys);
    DeleteFileA(driver->cat);
    DeleteFileA(driver->inf);
    RemoveDirectoryA(driver->dir);
}

tuna_error
tuna_install_embedded_driver(tuna_embedded_driver const *embedded_driver) {
    int extruded = 0;
    HDEVINFO dev_info = INVALID_HANDLE_VALUE;
    int registered = 0;
    
    tuna_error err = 0;
    
    tuna_extruded_driver extruded_driver;
    if ((err = tuna_extrude_driver(&extruded_driver, embedded_driver))) {
        goto out;
    }
    extruded = 1;

    GUID class_guid;
    char class_name[MAX_CLASS_NAME_LEN];
    if (!SetupDiGetINFClassA(extruded_driver.inf,
                             &class_guid,
                             class_name,
                             sizeof(class_name),
                             NULL))
    {
        err = tuna_translate_err_code(GetLastError());
        goto out;
    }

    dev_info = SetupDiCreateDeviceInfoList(&class_guid, NULL);
    if (dev_info == INVALID_HANDLE_VALUE) {
        err = tuna_translate_err_code(GetLastError());
        goto out;
    }

    SP_DEVINFO_DATA dev_info_data = {.cbSize = sizeof(dev_info_data)};
    if (!SetupDiCreateDeviceInfoA(dev_info,
                                  class_name, &class_guid,
                                  NULL,
                                  NULL,
                                  DICD_GENERATE_ID,
                                  &dev_info_data))
    {
        err = tuna_translate_err_code(GetLastError());
        goto out;
    }

    char hardware_id[MAX_PATH]; {
        char const *sys_name = embedded_driver->sys->name;
        size_t len = PathFindExtensionA(sys_name) - sys_name;
        memcpy(hardware_id, sys_name, len);
        memset(hardware_id + len, 0, 2);
    }

    if (!SetupDiSetDeviceRegistryProperty(dev_info, &dev_info_data,
                                          SPDRP_HARDWAREID,
                                          (BYTE const *)&hardware_id,
                                          (DWORD)(strlen(hardware_id) + 2)))
    {
        err = tuna_translate_err_code(GetLastError());
        goto out;
    }

    if (!SetupDiCallClassInstaller(DIF_REGISTERDEVICE,
                                   dev_info, &dev_info_data))
    {
        err = tuna_translate_err_code(GetLastError());
        goto out;
    }
    registered = 1;

    BOOL need_reboot;
    if (!UpdateDriverForPlugAndPlayDevicesA(NULL,
                                            hardware_id,
                                            extruded_driver.inf,
                                            0,
                                            &need_reboot))
    {
        err = tuna_translate_err_code(GetLastError());
        goto out;
    }
    if (need_reboot) {
        err = TUNA_UNEXPECTED;
        goto out;
    }

    char const new_hardware_id[] = "footun\0";
    if (!SetupDiSetDeviceRegistryProperty(dev_info, &dev_info_data,
                                          SPDRP_HARDWAREID,
                                          (BYTE const *)&new_hardware_id,
                                          sizeof(new_hardware_id)))
    {
        err = tuna_translate_err_code(GetLastError());
        goto out;
    }

  out:;
    if (err && registered) {
        SetupDiCallClassInstaller(DIF_REMOVE, dev_info, &dev_info_data);
    }
    if (dev_info != INVALID_HANDLE_VALUE) {
        SetupDiDestroyDeviceInfoList(dev_info);
    }
    if (extruded) { tuna_erase_extruded_driver(&extruded_driver); }
    return err;
}
