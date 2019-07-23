#define WIN32_LEAN_AND_MEAN

#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>

#include <windows.h>
#include <shellapi.h>
#include <setupapi.h>

///////////////////////////////////////////////////////////////////////////////

#pragma warning(disable: 4996) // No, MSVC, C functions are not deprecated.

static
HANDLE
tuna_parse_handle_arg(wchar_t const *arg) {
    uintptr_t handle;
    if (swscanf(arg, L"%"SCNuPTR, &handle) != 1) {
        ExitProcess(ERROR_ASSERTION_FAILURE);
    }
    return (HANDLE)handle;
}

static
void
tuna_parse_args(wchar_t **driver_instance_id,
                HANDLE *initialized_event,
                HANDLE *detached_event,
                HANDLE *parent_process)
{
    wchar_t **args;
    int arg_count;
    if (!(args = CommandLineToArgvW(GetCommandLineW(), &arg_count))) {
        ExitProcess(GetLastError());
    }

    if (!(*driver_instance_id = _wcsdup(args[1]))) {
        ExitProcess(ERROR_OUTOFMEMORY);
    }
    *initialized_event = tuna_parse_handle_arg(args[2]);
    *detached_event = tuna_parse_handle_arg(args[3]);
    *parent_process = tuna_parse_handle_arg(args[4]);

    LocalFree(args);
}

static
void
tuna_open_driver_instance(wchar_t const *driver_instance_id,
                          HANDLE *devinfo, SP_DEVINFO_DATA *devinfo_data)
{
    *devinfo = SetupDiCreateDeviceInfoList(NULL, NULL);
    if (*devinfo == INVALID_HANDLE_VALUE) {
        ExitProcess(GetLastError());
    }

    devinfo_data->cbSize = sizeof(*devinfo_data);
    if (!SetupDiOpenDeviceInfoW(*devinfo, driver_instance_id, NULL, 0,
                                devinfo_data))
    { ExitProcess(GetLastError()); }
}

int main(void) {
    wchar_t *driver_instance_id;
    HANDLE parent_process;
    HANDLE detached_event;
    HANDLE initialized_event;
    tuna_parse_args(&driver_instance_id,
                    &initialized_event,
                    &detached_event,
                    &parent_process);

    HDEVINFO devinfo;
    SP_DEVINFO_DATA devinfo_data;
    tuna_open_driver_instance(driver_instance_id, &devinfo, &devinfo_data);

    if (!SetEvent(initialized_event)) {
        ExitProcess(GetLastError());
    }
    CloseHandle(initialized_event);

    HANDLE awaited[] = {
        parent_process,
        detached_event,
    };
    switch (WaitForMultipleObjects(_countof(awaited), awaited,
                                   FALSE, INFINITE))
    {
    case WAIT_OBJECT_0 + 0:
    case WAIT_OBJECT_0 + 1:
        break;
    case WAIT_FAILED:
        ExitProcess(GetLastError());
    default:
        ExitProcess(ERROR_ASSERTION_FAILURE);
    }
    CloseHandle(parent_process);
    CloseHandle(detached_event);

    SetupDiCallClassInstaller(DIF_REMOVE, devinfo, &devinfo_data);
    SetupDiDestroyDeviceInfoList(devinfo);

    return 0;
}
