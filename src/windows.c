#include <tuna.h>

#include <wchar.h>
#include <limits.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>

#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <pathcch.h>
#include <cfgmgr32.h>
#include <setupapi.h>
#include <newdev.h>

#include <tap-windows.h>

#include <tuna/priv/windows/embedded_driver.h>

///////////////////////////////////////////////////////////////////////////////

#pragma warning(disable: 4996) // No, MSVC, C functions are not deprecated.

struct tuna_device {
    HANDLE handle;
    HANDLE janitor_handle;
};

static
void
tuna_deinitialize_device(tuna_device *device) {
    if (device->janitor_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(device->janitor_handle);
    }
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

static
tuna_error
tuna_translate_sys_error(DWORD err_code) {
    switch (err_code) {
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
tuna_error
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
tuna_error
tuna_join_paths(wchar_t const *base, wchar_t const *extra,
                wchar_t **joined_out)
{
    wchar_t *local_joined = NULL;
    wchar_t *joined = NULL;

    tuna_error err = 0;
    HRESULT hres;

    if ((hres = PathAllocCombine(base, extra, PATHCCH_ALLOW_LONG_PATHS,
                                 &local_joined)))
    {
        local_joined = NULL;
        err = tuna_translate_hresult(hres);
        goto out;
    }

    if (!(joined = _wcsdup(local_joined))) {
        err = TUNA_OUT_OF_MEMORY;
        goto out;
    }

    *joined_out = joined; joined = NULL;

out:
    free(joined);
    LocalFree(local_joined);

    return err;
}

static
tuna_error
tuna_extrude_file(tuna_embedded_file const *embedded, wchar_t const *dir,
                  wchar_t **path_out, int dry_run)
{
    wchar_t *path = NULL;
    HANDLE handle = INVALID_HANDLE_VALUE;

    tuna_error err = 0;

    if ((err = tuna_join_paths(dir, embedded->name, &path))) {
        goto out;
    }

    if (!dry_run) {
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

static
tuna_error
tuna_extrude_driver(tuna_embedded_driver const *embedded, wchar_t const *dir,
                    wchar_t **inf_path_out, int dry_run)
{
    wchar_t *inf_path = NULL;

    tuna_error err = 0;

    if ((err = tuna_extrude_file(embedded->inf_file, dir, &inf_path, dry_run))
     || (err = tuna_extrude_file(embedded->cat_file, dir, NULL, dry_run))
     || (err = tuna_extrude_file(embedded->sys_file, dir, NULL, dry_run)))
    { goto out; }

    *inf_path_out = inf_path; inf_path = NULL;

out:
    free(inf_path);

    return err;
}

static
tuna_error
tuna_get_local_app_data_dir(wchar_t **dir_out) {
    wchar_t *co_task_dir = NULL;
    wchar_t *dir = NULL;

    tuna_error err = 0;
    HRESULT hres;

    if ((hres = SHGetKnownFolderPath(&FOLDERID_LocalAppData,
                                     KF_FLAG_CREATE,
                                     NULL,
                                     &co_task_dir)))
    {
        co_task_dir = NULL;
        err = tuna_translate_hresult(hres);
        goto out;
    }

    if (!(dir = _wcsdup(co_task_dir))) {
        err = TUNA_OUT_OF_MEMORY;
        goto out;
    }

    *dir_out = dir; dir = NULL;

out:
    free(dir);
    CoTaskMemFree(co_task_dir);

    return err;
}

static
tuna_error
tuna_ensure_dir_exists(wchar_t const *dir) {
    if (!CreateDirectoryW(dir, NULL)) {
        DWORD err_code = GetLastError();
        if (err_code != ERROR_ALREADY_EXISTS) {
            return tuna_translate_sys_error(err_code);
        }
    }
    return 0;
}

#define TUNA_STRINGIFY_INDIRECT(token) \
    #token \
/**/
#define TUNA_STRINGIFY(token) \
    TUNA_STRINGIFY_INDIRECT(token) \
/**/

#define TUNA_VERSION_STRING \
    L"" TUNA_STRINGIFY(TUNA_MAJOR_VERSION) \
    "." TUNA_STRINGIFY(TUNA_MINOR_VERSION) \
    "." TUNA_STRINGIFY(TUNA_PATCH_VERSION) \
/**/

#if UINTPTR_MAX == 0xFFFFFFFF
    #define TUNA_ARCH_STRING L"32"
#else
    #define TUNA_ARCH_STRING L"64"
#endif

#define TUNA_EXTRUSION_MUTEX_NAME \
    L"Global\\tuna-" TUNA_VERSION_STRING "-" TUNA_ARCH_STRING "-extrusion" \
/**/

static
tuna_error
tuna_path_exists(wchar_t const *path, int *exists_out) {
    if (GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES) {
        DWORD err_code = GetLastError();
        if (err_code != ERROR_FILE_NOT_FOUND) {
            return tuna_translate_sys_error(err_code);
        }
        *exists_out = 0;
    } else {
        *exists_out = 1;
    }
    return 0;
}

extern tuna_embedded_file const tuna_priv_embedded_janitor;

static
tuna_error
tuna_ensure_extruded(tuna_embedded_driver const *embedded_driver,
                     wchar_t **driver_inf_path_out, wchar_t **janitor_path_out)
{
    wchar_t *local_app_data_dir = NULL;
    wchar_t *base_dir = NULL;
    wchar_t *version_dir = NULL;
    wchar_t *arch_dir = NULL;
    HANDLE mutex = NULL;
    wchar_t *marker_path = NULL;
    wchar_t *driver_inf_path = NULL;
    wchar_t *janitor_path = NULL;
    HANDLE marker_handle = INVALID_HANDLE_VALUE;

    tuna_error err = 0;

    if ((err = tuna_get_local_app_data_dir(&local_app_data_dir))
     || (err = tuna_join_paths(local_app_data_dir, L"tuna", &base_dir))
     || (err = tuna_ensure_dir_exists(base_dir))
     || (err = tuna_join_paths(base_dir, TUNA_VERSION_STRING, &version_dir))
     || (err = tuna_ensure_dir_exists(version_dir))
     || (err = tuna_join_paths(version_dir, TUNA_ARCH_STRING, &arch_dir))
     || (err = tuna_ensure_dir_exists(arch_dir)))
    { goto out; }

    if (!(mutex = CreateMutexW(NULL, FALSE, TUNA_EXTRUSION_MUTEX_NAME))) {
        err = tuna_translate_sys_error(GetLastError());
        goto out;
    }

    switch (WaitForSingleObject(mutex, INFINITE)) {
    case WAIT_ABANDONED:
    case WAIT_OBJECT_0:
        break;
    default:
        err = tuna_translate_sys_error(GetLastError());
        goto out;
    }

    int extruded;
    if ((err = tuna_join_paths(arch_dir, L"extrusion_marker", &marker_path))
     || (err = tuna_path_exists(marker_path, &extruded))
     || (err = tuna_extrude_driver(embedded_driver, arch_dir,
                                   &driver_inf_path, extruded))
     || (err = tuna_extrude_file(&tuna_priv_embedded_janitor, arch_dir,
                                 &janitor_path, extruded)))
    { goto out; }
    if (!extruded) {
        if ((marker_handle = CreateFileW(marker_path,
                                         0,
                                         0,
                                         NULL,
                                         CREATE_ALWAYS,
                                         FILE_ATTRIBUTE_NORMAL,
                                         NULL)) == INVALID_HANDLE_VALUE)
        {
            err = tuna_translate_sys_error(GetLastError());
            goto out;
        }
    }

    *driver_inf_path_out = driver_inf_path; driver_inf_path = NULL;
    *janitor_path_out = janitor_path; janitor_path = NULL;

out:
    if (marker_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(marker_handle);
    }
    free(janitor_path);
    free(driver_inf_path);
    free(marker_path);
    if (mutex) {
        ReleaseMutex(mutex);
        CloseHandle(mutex);
    }
    free(arch_dir);
    free(version_dir);
    free(base_dir);
    free(local_app_data_dir);

    return err;
}

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



static
tuna_error
tuna_install_driver(wchar_t const *inf_path, wchar_t const *hardware_id,
                    HANDLE *janitor_handle)
{
//    tuna_extruded_driver extruded = {0};
//    HDEVINFO dev_info = INVALID_HANDLE_VALUE;
//    _Bool registered = 0;
    
    tuna_error err = 0;
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
//out:
//    if (err && registered) {
//        SetupDiCallClassInstaller(DIF_REMOVE, dev_info, &dev_info_data);
//    }
//    if (dev_info != INVALID_HANDLE_VALUE) {
//        SetupDiDestroyDeviceInfoList(dev_info);
//    }
//    tuna_erase_extruded_driver(&extruded);
//
    return err;
}

#define TUNA_DEVICE_INITIALIZER (tuna_device){ \
    .handle = INVALID_HANDLE_VALUE, \
    .janitor_handle = INVALID_HANDLE_VALUE, \
}

static
tuna_error
tuna_allocate_device(tuna_device **device_out) {
    tuna_device *device;
    if (!(device = malloc(sizeof(*device)))) {
        return TUNA_OUT_OF_MEMORY;
    }
    *device = TUNA_DEVICE_INITIALIZER;

    *device_out = device;

    return 0;
}

extern tuna_embedded_driver const tuna_priv_embedded_tap_windows;

tuna_error
tuna_create_device(tuna_device **device_out, tuna_ownership ownership) {
    wchar_t *driver_inf_path = NULL;
    wchar_t *janitor_path = NULL;
    tuna_device *device = NULL;

    tuna_error err = 0;

    if (((ownership == TUNA_SHARED) && (err = TUNA_UNSUPPORTED))
     || (err = tuna_ensure_extruded(&tuna_priv_embedded_tap_windows,
                                    &driver_inf_path, &janitor_path))
     || (err = tuna_allocate_device(&device))
     || (err = tuna_install_driver(driver_inf_path,
                                   tuna_priv_embedded_tap_windows.hardware_id,
                                   &device->janitor_handle)))
    { goto out; }

    *device_out = device; device = NULL;

out:
    tuna_free_device(device);
    free(janitor_path);
    free(driver_inf_path);

    return err;
}
