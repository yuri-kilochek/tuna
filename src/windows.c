#include <tuna.h>

#include <wchar.h>
#include <stdlib.h>
#include <stdint.h>
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

#define TUNA_NO_ANCESTOR_DIR (-1)

static
int
tuna_translate_error(DWORD err_code) {
    switch (err_code) {
      case ERROR_PATH_NOT_FOUND:;
        return TUNA_NO_ANCESTOR_DIR;
      case 0:;
        return 0;
      case ERROR_ACCESS_DENIED:;
      case ERROR_AUTHENTICODE_PUBLISHER_NOT_TRUSTED:;
        return TUNA_FORBIDDEN;
      case ERROR_NOT_ENOUGH_MEMORY:;
      case ERROR_OUTOFMEMORY:;
        return TUNA_OUT_OF_MEMORY;
      case ERROR_TOO_MANY_OPEN_FILES:;
        return TUNA_TOO_MANY_HANDLES;
      default:;
        return TUNA_UNEXPECTED;
    }
}

static
int
tuna_translate_hresult(HRESULT hres) {
    switch (HRESULT_FACILITY(hres)) {
      case FACILITY_WIN32:
        return tuna_translate_error(HRESULT_CODE(hres));
      default:
        switch (hres) {
          case 0:;
            return 0;
          default:;
            return TUNA_UNEXPECTED;
        }
    }
}

static
int
tuna_join_paths(wchar_t **joined_out,
                wchar_t const *base, wchar_t const *extra)
{
    wchar_t *local_joined = NULL;
    wchar_t *joined = NULL;

    int err = 0;

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
tuna_unextrude_file(HANDLE handle) {
    if (handle != INVALID_HANDLE_VALUE) { CloseHandle(handle); }
}

static
int
tuna_extrude_file(HANDLE *handle_out,
                  wchar_t const *dir,
                  tuna_embedded_file const *embedded)
{
    wchar_t *path = NULL;
    HANDLE handle = INVALID_HANDLE_VALUE;

    int err = 0;

    if ((err = tuna_join_paths(&path, dir, embedded->name))) {
        goto out;
    }
    handle = CreateFileW(path,
                         GENERIC_WRITE,
                         FILE_SHARE_READ,
                         NULL,
                         CREATE_ALWAYS,
                         FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE,
                         NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        err = tuna_translate_error(GetLastError());
        goto out;
    }

    if (!WriteFile(handle,
                   embedded->data, (DWORD)embedded->size,
                   &(DWORD){0},
                   NULL))
    {
        err = tuna_translate_error(GetLastError());
        goto out;
    }

    if (!FlushFileBuffers(handle)) {
        err = tuna_translate_error(GetLastError());
        goto out;
    }

    *handle_out = handle;

  out:;
    if (err) { tuna_unextrude_file(handle); }
    free(path);

    return err;
}

static
int
tuna_get_extrusion_dir(wchar_t **dir_out) {
    wchar_t temp_dir[MAX_PATH + 1];
    if (!GetTempPathW(_countof(temp_dir), temp_dir)) {
        return tuna_translate_error(GetLastError());
    }
    return tuna_join_paths(dir_out, temp_dir, L"TunaDrivers");
}

typedef struct {
    HANDLE inf_handle;
    HANDLE cat_handle;
    HANDLE sys_handle;
} tuna_extruded_driver;

#define TUNA_EXTRUDED_DRIVER_INIT { \
    .inf_handle = INVALID_HANDLE_VALUE, \
    .cat_handle = INVALID_HANDLE_VALUE, \
    .sys_handle = INVALID_HANDLE_VALUE, \
}

static
void
tuna_unextrude_driver(tuna_extruded_driver const *extruded) {
    wchar_t *dir = NULL;
    wchar_t *subdir = NULL;
    HANDLE handle = INVALID_HANDLE_VALUE;

    tuna_unextrude_file(extruded->sys_handle);
    tuna_unextrude_file(extruded->cat_handle);
    tuna_unextrude_file(extruded->inf_handle);

    if (tuna_get_extrusion_dir(&dir)) { goto out; }

    if (tuna_join_paths(&subdir, dir, L"*")) { goto out; }

    WIN32_FIND_DATAW data;
    wchar_t *name = data.cFileName;
    handle = FindFirstFileExW(subdir,
                              FindExInfoBasic,
                              &data,
                              FindExSearchLimitToDirectories,
                              NULL,
                              0);
    if (handle == INVALID_HANDLE_VALUE) { goto out; }
    do {
        if (!wcscmp(name, L".")) { continue; }
        if (!wcscmp(name, L"..")) { continue; }

        free(subdir); subdir = NULL;
        if (tuna_join_paths(&subdir, dir, name)) { goto out; }

        RemoveDirectoryW(subdir);
    } while (FindNextFileW(handle, &data));

    RemoveDirectoryW(dir);

  out:;
    if (handle != INVALID_HANDLE_VALUE) { FindClose(handle); };
    free(subdir);
    free(dir);
}

static
int
tuna_extrude_driver(tuna_extruded_driver *extruded_out,
                    tuna_embedded_driver const *embedded)
{
    wchar_t *dir = NULL;
    wchar_t *subdir = NULL;
    tuna_extruded_driver extruded = TUNA_EXTRUDED_DRIVER_INIT;

    int err = 0;

    if ((err = tuna_get_extrusion_dir(&dir))) { goto out; }

    wchar_t subdir_name[sizeof(uintmax_t) * 2 + 1];
    if (swprintf(subdir_name, _countof(subdir_name),
                 L"%jx", (uintmax_t)GetCurrentThreadId()) < 0)
    {
        err = TUNA_UNEXPECTED;
        goto out;
    }
    if ((err = tuna_join_paths(&subdir, dir, subdir_name))) { goto out; }

  create_dir:;
    if (!CreateDirectoryW(dir, NULL)) {
        DWORD err_code = GetLastError();
        switch (err_code) {
          case ERROR_ALREADY_EXISTS:;
            break;
          default:;
            err = tuna_translate_error(err_code);
            goto out;
        }
    }

  create_subdir:
    if (!CreateDirectoryW(subdir, NULL)) {
        DWORD err_code = GetLastError();
        switch (err_code) {
          case ERROR_PATH_NOT_FOUND:;
            goto create_dir;
          case ERROR_ALREADY_EXISTS:;
            break;
          default:;
            err = tuna_translate_error(err_code);
            goto out;
        }
    }

    switch ((err = tuna_extrude_file(&extruded.inf_handle,
                                     subdir, embedded->inf_file)))
    {
      case TUNA_NO_ANCESTOR_DIR:;
        goto create_subdir;
      case 0:;
        break;
      default:;
        goto out;
    }
    if ((err = tuna_extrude_file(&extruded.cat_handle,
                                 subdir, embedded->cat_file))) { goto out; }
    if ((err = tuna_extrude_file(&extruded.sys_handle,
                                 subdir, embedded->sys_file))) { goto out; }

    *extruded_out = extruded;

  out:;
    if (err) { tuna_unextrude_driver(&extruded); }

    return err;
}

//static
//tuna_error
//tuna_get_device_text_property(wchar_t **value_out,
//                              HDEVINFO dev_info,
//                              SP_DEVINFO_DATA *dev_info_data,
//                              DWORD key)
//{
//    wchar_t *value = NULL;
//
//    tuna_error err = 0;
//
//    DWORD type;
//    DWORD size;
//    if (!SetupDiGetDeviceRegistryPropertyW(dev_info, dev_info_data,
//                                           key, &type,
//                                           NULL, 0, &size))
//    {
//        DWORD err_code = GetLastError();
//        if (err_code == ERROR_INVALID_DATA) { goto done; }
//        err = tuna_translate_error(err_code);
//        goto out;
//    }
//    switch (type) {
//      case REG_SZ:;
//      case REG_MULTI_SZ:;
//        break;
//      default:;
//        err = TUNA_UNEXPECTED;
//        goto out;
//    }
//
//    if (!(value = malloc(size))) {
//        err = TUNA_OUT_OF_MEMORY;
//        goto out;
//    }
//
//    if (!SetupDiGetDeviceRegistryPropertyW(dev_info, dev_info_data,
//                                           key, NULL,
//                                           (void *)value, size, NULL))
//    {
//        err = tuna_translate_error(GetLastError());
//        goto out;
//    }
//
//  done:;
//    *value_out = value;
//
//  out:;
//    if (err) { free(value); }
//
//    return err;
//}
//
//
//static
//tuna_error
//tuna_find_devices(HDEVINFO *dev_info_out, wchar_t const *hardware_id) {
//    HDEVINFO dev_info = INVALID_HANDLE_VALUE;
//    wchar_t *hwids = NULL;
//    size_t hwids_byte_size = 0;
//    
//    tuna_error err = 0;
//
//    dev_info = SetupDiGetClassDevsW(NULL, NULL, NULL, DIGCF_ALLCLASSES);
//    if (dev_info == INVALID_HANDLE_VALUE) {
//        err = tuna_translate_error(GetLastError());
//        goto out;
//    }
//
//    for (DWORD i = 0;;) {
//        SP_DEVINFO_DATA dev_info_data = {.cbSize = sizeof(dev_info_data)};
//        if (!SetupDiEnumDeviceInfo(dev_info, i, &dev_info_data)) {
//            DWORD err_code = GetLastError();
//            if (err_code == ERROR_NO_MORE_ITEMS) { break; }
//            err = tuna_translate_error(err_code);
//            goto out;
//        }
//
//        DWORD required_hwids_byte_size;
//        if (!SetupDiGetDeviceRegistryPropertyW(dev_info, &dev_info_data,
//                                               SPDRP_HARDWAREID, NULL,
//                                               NULL, 0,
//                                               &required_hwids_byte_size))
//        {
//            DWORD err_code = GetLastError();
//            if (err_code == ERROR_INVALID_DATA) { goto remove; }
//            err = tuna_translate_error(err_code);
//            goto out;
//        }
//        if (required_hwids_byte_size > hwids_byte_size) {
//            free(hwids);
//            if (!(hwids = malloc(required_hwids_byte_size))) {
//                err = TUNA_OUT_OF_MEMORY;
//                goto out;
//            }
//            hwids_byte_size = required_hwids_byte_size;
//        }
//        if (!SetupDiGetDeviceRegistryPropertyW(dev_info, &dev_info_data,
//                                               SPDRP_HARDWAREID, NULL,
//                                               (BYTE *)hwids,
//                                               (DWORD)hwids_byte_size,
//                                               NULL))
//        {
//            err = tuna_translate_error(GetLastError());
//            goto out;
//        }
//
//        for (wchar_t *hwid = hwids; *hwid; hwid += wcslen(hwid) + 1) {
//            if (wcscmp(hwid, hardware_id)) { goto keep; }
//        }
//
//      remove:;
//        if (!SetupDiDeleteDeviceInfo(dev_info, &dev_info_data)) {
//            err = tuna_translate_error(GetLastError());
//            goto out;
//        }
//        continue;
//      keep:;
//        ++i;
//    }
//
//    *dev_info_out = dev_info;
//
//  out:;
//    free(hwids);
//    if (err && dev_info != INVALID_HANDLE_VALUE) {
//        SetupDiDestroyDeviceInfoList(dev_info);
//    }
//
//    return err;
//}
//
//static
//tuna_error
//tuna_install_driver(tuna_embedded_driver const *embedded,
//                    wchar_t const *hardware_id)
//{
//    tuna_extruded_driver extruded = {0};
//    HDEVINFO dev_info = INVALID_HANDLE_VALUE;
//    _Bool registered = 0;
//    
//    tuna_error err = 0;
//    
//    if ((err = tuna_extrude_driver(&extruded, embedded))) { goto out; }
//
//    GUID class_guid;
//    wchar_t class_name[MAX_CLASS_NAME_LEN];
//    if (!SetupDiGetINFClassW(extruded.inf_path,
//                             &class_guid,
//                             class_name,
//                             sizeof(class_name) / sizeof(*class_name),
//                             NULL))
//    {
//        err = tuna_translate_error(GetLastError());
//        goto out;
//    }
//
//    dev_info = SetupDiCreateDeviceInfoList(&class_guid, NULL);
//    if (dev_info == INVALID_HANDLE_VALUE) {
//        err = tuna_translate_error(GetLastError());
//        goto out;
//    }
//
//    SP_DEVINFO_DATA dev_info_data = {.cbSize = sizeof(dev_info_data)};
//    if (!SetupDiCreateDeviceInfoW(dev_info,
//                                  class_name, &class_guid,
//                                  NULL,
//                                  NULL,
//                                  DICD_GENERATE_ID,
//                                  &dev_info_data))
//    {
//        err = tuna_translate_error(GetLastError());
//        goto out;
//    }
//
//    assert(wcslen(hardware_id) <= MAX_DEVICE_ID_LEN);
//    wchar_t hardware_ids[MAX_DEVICE_ID_LEN + 1 + 1];
//    wcscpy(hardware_ids, hardware_id);
//    hardware_ids[wcslen(hardware_ids) + 1] = L'\0';
//
//    if (!SetupDiSetDeviceRegistryPropertyW(dev_info, &dev_info_data,
//                                           SPDRP_HARDWAREID,
//                                           (BYTE *)&hardware_ids,
//                                           (DWORD)sizeof(hardware_ids)))
//    {
//        err = tuna_translate_error(GetLastError());
//        goto out;
//    }
//
//    if (!SetupDiCallClassInstaller(DIF_REGISTERDEVICE,
//                                   dev_info, &dev_info_data))
//    {
//        err = tuna_translate_error(GetLastError());
//        goto out;
//    }
//    registered = 1;
//
//    BOOL need_reboot;
//    if (!UpdateDriverForPlugAndPlayDevicesW(NULL,
//                                            hardware_id,
//                                            extruded.inf_path,
//                                            0,
//                                            &need_reboot))
//    {
//        err = tuna_translate_error(GetLastError());
//        goto out;
//    }
//    assert(!need_reboot);
//
//  out:;
//    if (err && registered) {
//        SetupDiCallClassInstaller(DIF_REMOVE, dev_info, &dev_info_data);
//    }
//    if (dev_info != INVALID_HANDLE_VALUE) {
//        SetupDiDestroyDeviceInfoList(dev_info);
//    }
//    tuna_erase_extruded_driver(&extruded);
//
//    return err;
//}

extern tuna_embedded_driver const tuna_embedded_tap_windows;

struct tuna_device {
    HANDLE handle;
};

tuna_error
tuna_create_device(tuna_device **device) {
    tuna_device *dev = NULL;

    tuna_error err = 0;

    //if ((err = tuna_install_driver(&tuna_embedded_tap_windows,
    //                               TUNA_TAP_WINDOWS_HARDWARE_ID)))
    //{ goto fail; }

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
