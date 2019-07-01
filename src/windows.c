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

//#define TUNA_NO_SUCH_PROPERTY (-2)
//#define TUNA_NO_ANCESTOR_DIR (-1)

static
int
tuna_translate_sys_error(DWORD err_code) {
    switch (err_code) {
    //case ERROR_PATH_NOT_FOUND:
    //    return TUNA_NO_ANCESTOR_DIR;
    case 0:
        return 0;
    default:
        return TUNA_UNEXPECTED;
    case ERROR_ACCESS_DENIED:
    case ERROR_AUTHENTICODE_PUBLISHER_NOT_TRUSTED:
        return TUNA_FORBIDDEN;
    case ERROR_NOT_ENOUGH_MEMORY:
    case ERROR_OUTOFMEMORY:
        return TUNA_OUT_OF_MEMORY;
    case ERROR_TOO_MANY_OPEN_FILES:
        return TUNA_TOO_MANY_HANDLES;
    }
}

static
int
tuna_translate_hresult(HRESULT hres) {
    switch (HRESULT_FACILITY(hres)) {
    case FACILITY_WIN32:
        return tuna_translate_sys_error(HRESULT_CODE(hres));
    default:
        switch (hres) {
        case 0:
            return 0;
        default:
            return TUNA_UNEXPECTED;
        }
    }
}

static
int
tuna_join_paths(wchar_t const *base, wchar_t const *extra,
                wchar_t **joined_out)
{
    wchar_t *local_joined = NULL;
    wchar_t *joined = NULL;

    int err = 0;
    HRESULT hres;

    if ((hres = PathAllocCombine(base, extra, PATHCCH_ALLOW_LONG_PATHS,
                                 &local_joined)))
    {
        local_joined = NULL;
        err = tuna_translate_hresult(hres);
        goto out;
    }

    size_t joined_size = (wcslen(local_joined) + 1) * sizeof(*joined);
    if (!(joined = malloc(joined_size))) {
        err = TUNA_OUT_OF_MEMORY;
        goto out;
    }
    memcpy(joined, local_joined, joined_size);

    *joined_out = joined; joined = NULL;

out:
    free(joined);
    LocalFree(local_joined);

    return err;
}

static
int
tuna_extrude_file(tuna_embedded_file const *embedded, wchar_t const *dir,
                  wchar_t **path_out)
{
    wchar_t *path = NULL;
    HANDLE handle = INVALID_HANDLE_VALUE;

    int err = 0;

    if ((err = tuna_join_paths(embedded->name, dir, &path))) {
        goto out;
    }

    if ((handle = CreateFileW(path,
                              GENERIC_WRITE,
                              0,
                              NULL,
                              CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL)) == INVALID_HANDLE_VALUE
     || !WriteFile(handle,
                   embedded->data, (DWORD)embedded->size,
                   &(DWORD){0},
                   NULL)
     || !FlushFileBuffers(handle))
    {
        err = tuna_translate_sys_error(GetLastError());
        goto out;
    }

    if (path_out) {
        *path_out = path; path = NULL;
    }

out:
    free(path);
    if (handle != INVALID_HANDLE_VALUE) {
        CloseHandle(handle);
    }

    return err;
}
//
//static
//int
//tuna_get_extrusion_dir(wchar_t **dir_out) {
//    wchar_t temp_dir[MAX_PATH + 1];
//    if (!GetTempPathW(_countof(temp_dir), temp_dir)) {
//        return tuna_translate_error(GetLastError());
//    }
//    return tuna_join_paths(dir_out, temp_dir, L"TunaDrivers");
//}
//
//typedef struct {
//    wchar_t *inf_path;
//    HANDLE inf_handle;
//    HANDLE cat_handle;
//    HANDLE sys_handle;
//} tuna_extruded_driver;
//
//#define TUNA_EXTRUDED_DRIVER_INIT { \
//    .inf_handle = INVALID_HANDLE_VALUE, \
//    .cat_handle = INVALID_HANDLE_VALUE, \
//    .sys_handle = INVALID_HANDLE_VALUE, \
//}
//
//static
//void
//tuna_unextrude_driver(tuna_extruded_driver const *extruded) {
//    wchar_t *dir = NULL;
//    wchar_t *subdir = NULL;
//    HANDLE handle = INVALID_HANDLE_VALUE;
//
//    tuna_unextrude_file(extruded->inf_path, extruded->sys_handle);
//    tuna_unextrude_file(NULL, extruded->cat_handle);
//    tuna_unextrude_file(NULL, extruded->inf_handle);
//
//    if (tuna_get_extrusion_dir(&dir)) { goto out; }
//
//    if (tuna_join_paths(&subdir, dir, L"*")) { goto out; }
//
//    WIN32_FIND_DATAW data;
//    handle = FindFirstFileExW(subdir,
//                              FindExInfoBasic,
//                              &data,
//                              FindExSearchLimitToDirectories,
//                              NULL,
//                              0);
//    if (handle == INVALID_HANDLE_VALUE) { goto out; }
//    do {
//        wchar_t *name = data.cFileName;
//        if (!wcscmp(name, L".")) { continue; }
//        if (!wcscmp(name, L"..")) { continue; }
//
//        free(subdir); subdir = NULL;
//        if (tuna_join_paths(&subdir, dir, name)) { goto out; }
//
//        RemoveDirectoryW(subdir);
//    } while (FindNextFileW(handle, &data));
//
//    RemoveDirectoryW(dir);
//
//  out:;
//    if (handle != INVALID_HANDLE_VALUE) { FindClose(handle); };
//    free(subdir);
//    free(dir);
//}
//
//static
//int
//tuna_extrude_driver(tuna_extruded_driver *extruded_out,
//                    tuna_embedded_driver const *embedded)
//{
//    wchar_t *dir = NULL;
//    wchar_t *subdir = NULL;
//    tuna_extruded_driver extruded = TUNA_EXTRUDED_DRIVER_INIT;
//
//    int err = 0;
//
//    if ((err = tuna_get_extrusion_dir(&dir))) { goto out; }
//
//    wchar_t subdir_name[sizeof(uintmax_t) * 2 + 1];
//    if (swprintf(subdir_name, _countof(subdir_name),
//                 L"%jx", (uintmax_t)GetCurrentThreadId()) < 0)
//    {
//        err = TUNA_UNEXPECTED;
//        goto out;
//    }
//    if ((err = tuna_join_paths(&subdir, dir, subdir_name))) { goto out; }
//
//  create_dir:;
//    if (!CreateDirectoryW(dir, NULL)) {
//        DWORD err_code = GetLastError();
//        switch (err_code) {
//          case ERROR_ALREADY_EXISTS:;
//            break;
//          default:;
//            err = tuna_translate_error(err_code);
//            goto out;
//        }
//    }
//
//  create_subdir:;
//    if (!CreateDirectoryW(subdir, NULL)) {
//        DWORD err_code = GetLastError();
//        switch (err_code) {
//          case ERROR_PATH_NOT_FOUND:;
//            goto create_dir;
//          case ERROR_ALREADY_EXISTS:;
//            break;
//          default:;
//            err = tuna_translate_error(err_code);
//            goto out;
//        }
//    }
//
//    switch ((err = tuna_extrude_file(&extruded.inf_path, &extruded.inf_handle,
//                                     subdir, embedded->inf_file)))
//    {
//      case TUNA_NO_ANCESTOR_DIR:;
//        goto create_subdir;
//      case 0:;
//        break;
//      default:;
//        goto out;
//    }
//    if ((err = tuna_extrude_file(NULL, &extruded.cat_handle,
//                                 subdir, embedded->cat_file))) { goto out; }
//    if ((err = tuna_extrude_file(NULL, &extruded.sys_handle,
//                                 subdir, embedded->sys_file))) { goto out; }
//
//    *extruded_out = extruded;
//
//  out:;
//    if (err) { tuna_unextrude_driver(&extruded); }
//
//    return err;
//}
//
//typedef struct {
//    wchar_t **items;
//    size_t len;
//} tuna_string_list;
//
//#define TUNA_STRING_LIST_INIT {0}
//
//static
//void
//tuna_free_string_list(tuna_string_list const *list) {
//    for (size_t i = 0; i < list->len; ++i) { free(list->items[i]); }
//    free(list->items); 
//}
//
//static
//int
//tuna_parse_reg_multi_sz(tuna_string_list *list_out, wchar_t const *multi_sz) {
//    tuna_string_list list = TUNA_STRING_LIST_INIT;
//
//    int err = 0;
//
//    for (wchar_t const *sz = multi_sz; *sz; sz += wcslen(sz) + 1) {
//        ++list.len;
//    }
//
//    if (!(list.items = malloc((list.len + 1) * sizeof(*list.items)))) {
//        err = TUNA_OUT_OF_MEMORY;
//        goto out;
//    }
//
//    wchar_t const *sz = multi_sz;
//    for (size_t i = 0; i < list.len; ++i) {
//        size_t len = wcslen(sz);
//
//        wchar_t **str = &list.items[i];
//        if (!(*str = malloc((len + 1) * sizeof(**str)))) {
//            err = TUNA_OUT_OF_MEMORY;
//            goto out;
//        }
//        wcscpy(*str, sz);
//
//        sz += len + 1;
//    }
//
//    *list_out = list;
//
//  out:;
//    if (err) { tuna_free_string_list(&list); }
//
//    return err;
//}
//
//static
//int
//tuna_render_reg_multi_sz(wchar_t **multi_sz_out,
//                         tuna_string_list const *list)
//{
//    wchar_t *multi_sz = NULL;
//
//    int err = 0;
//
//    size_t len = 0;
//    for (size_t i = 0; i < list->len; ++i) {
//        len += wcslen(list->items[i]) + 1;
//    }
//
//    if (!(multi_sz = malloc((len + 1) * sizeof(*multi_sz)))) {
//        err = TUNA_OUT_OF_MEMORY;
//        goto out;
//    }
//
//    wchar_t *sz = multi_sz;
//    for (size_t i = 0; i < list->len; ++i) {
//        wchar_t const *str = list->items[i];
//        size_t len = wcslen(str);
//        if (len == 0) { continue; }
//        wcscpy(sz, str);
//        sz += len + 1;
//    }
//    multi_sz[len] = L'\0';
//
//    *multi_sz_out = multi_sz;
//
//  out:;
//    if (err) { free(multi_sz); }
//
//    return err;
//}
//
//static
//int
//tuna_get_device_string_list_property(tuna_string_list *list_out,
//                                     HDEVINFO dev_info,
//                                     SP_DEVINFO_DATA *dev_info_data,
//                                     DWORD property)
//{
//    wchar_t *multi_sz = NULL;
//    tuna_string_list list = TUNA_STRING_LIST_INIT;
//
//    int err = 0;
//
//    DWORD size;
//    if (!SetupDiGetDeviceRegistryPropertyW(dev_info, dev_info_data,
//                                           property, NULL,
//                                           NULL, 0, &size))
//    {
//        DWORD err_code = GetLastError();
//        if (err_code == ERROR_INVALID_DATA) {
//            err = TUNA_NO_SUCH_PROPERTY;
//        } else {
//            err = tuna_translate_error(err_code);
//        }
//        goto out;
//    }
//    size_t len = (size + sizeof(*multi_sz) - 1) / sizeof(*multi_sz) + 1;
//    
//    if (!(multi_sz = malloc((len + 1) * sizeof(*multi_sz)))) {
//        err = TUNA_OUT_OF_MEMORY;
//        goto out;
//    }
//
//    if (!SetupDiGetDeviceRegistryPropertyW(dev_info, dev_info_data,
//                                           property, NULL,
//                                           (void *)multi_sz, size, NULL))
//    {
//        err = tuna_translate_error(GetLastError());
//        goto out;
//    }
//    multi_sz[len - 1] = multi_sz[len] = L'\0';
//
//    if ((err = tuna_parse_reg_multi_sz(&list, multi_sz))) { goto out; }
//
//    *list_out = list;
//
//  out:;
//    if (err) { tuna_free_string_list(&list); }
//    free(multi_sz);
//
//    return err;
//}
//
//static
//int
//tuna_set_device_string_list_property(HDEVINFO dev_info,
//                                     SP_DEVINFO_DATA *dev_info_data,
//                                     DWORD property,
//                                     tuna_string_list const *list)
//{
//    wchar_t *multi_sz = NULL;
//
//    int err = 0;
//
//    if ((err = tuna_render_reg_multi_sz(&multi_sz, list))) { goto out; }
//
//    size_t len = 0;
//    while (multi_sz[len]) { len += wcslen(multi_sz + len) + 1; }
//    size_t size = (len + 1) * sizeof(*multi_sz);
//
//    if (!SetupDiSetDeviceRegistryPropertyW(dev_info, dev_info_data, property,
//                                           (void *)multi_sz, (DWORD)size))
//    {
//        err = tuna_translate_error(GetLastError());
//        goto out;
//    }
//
//  out:;
//    free(multi_sz);
//
//    return err;
//}
//
//static
//int
//tuna_replace_device_hardware_id(HDEVINFO dev_info,
//                                SP_DEVINFO_DATA *dev_info_data,
//                                wchar_t const *cur_hwid,
//                                wchar_t const *new_hwid)
//{
//    tuna_string_list hwids = TUNA_STRING_LIST_INIT;
//
//    int err = 0;
//
//    if ((err = tuna_get_device_string_list_property(&hwids,
//                                                    dev_info, dev_info_data,
//                                                    SPDRP_HARDWAREID)))
//    { goto out; }
//
//    _Bool changed = 0;
//    for (size_t i = 0; i < hwids.len; ++i) {
//        wchar_t **hwid = &hwids.items[i];
//        if (wcscmp(*hwid, cur_hwid)) { continue; }
//        free(*hwid);
//        if (!(*hwid = _wcsdup(new_hwid))) {
//            err = TUNA_OUT_OF_MEMORY;
//            goto out;
//        }
//        changed = 1;
//    }
//
//    if (changed) {
//        if ((err = tuna_set_device_string_list_property(dev_info,
//                                                        dev_info_data,
//                                                        SPDRP_HARDWAREID,
//                                                        &hwids)))
//        { goto out; }
//    }
//
//  out:;
//    tuna_free_string_list(&hwids);
//
//    return err;
//}
//
//static
//int
//tuna_replace_every_devices_hardware_id(wchar_t const *cur_hwid,
//                                       wchar_t const *new_hwid,
//                                       _Bool relaxed)
//{
//    HDEVINFO dev_info = INVALID_HANDLE_VALUE;
//
//    int err = 0;
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
//        if ((err = tuna_replace_device_hardware_id(dev_info, &dev_info_data,
//                                                   cur_hwid, new_hwid)))
//        { if (!relaxed) { goto out; } }
//    }
//
//  out:;
//    if (dev_info != INVALID_HANDLE_VALUE) {
//        SetupDiDestroyDeviceInfoList(dev_info);
//    }
//
//    return (!relaxed) ? err : 0;
//}
//
//static wchar_t const tuna_quote_prefix[] = L"tuna-quoted-";



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

//tuna_error
//tuna_create_device(tuna_device **device) {
//    tuna_device *dev = NULL;
//
//    tuna_error err = 0;
//
//    //if ((err = tuna_install_driver(&tuna_embedded_tap_windows,
//    //                               TUNA_TAP_WINDOWS_HARDWARE_ID)))
//    //{ goto fail; }
//
//    if (!(dev = malloc(sizeof(*dev)))) {
//        err = TUNA_OUT_OF_MEMORY;
//        goto fail;
//    }
//    *dev = (tuna_device){.handle = INVALID_HANDLE_VALUE};
//
//
//
//
//
//    *device = dev;
//
//  done:;
//    return err;
//  fail:;
//    tuna_destroy_device(dev);
//    goto done;
//}

extern tuna_embedded_driver const tuna_priv_embedded_tap_windows;
extern tuna_embedded_file const tuna_priv_embedded_janitor;

struct tuna_device {
    HANDLE handle;
};

#define TUNA_DEVICE_INITIALIZER (tuna_device){ \
    .handle = INVALID_HANDLE_VALUE, \
}

static
void
tuna_deinitialize_device(tuna_device *device) {
    if (device->handle != INVALID_HANDLE_VALUE) {
        CloseHandle(device->handle);
    }
}

void
tuna_free_device(tuna_device *device) {
    if (device) {
        tuna_deinitialize_device(device);
        free(device);
    }
}
