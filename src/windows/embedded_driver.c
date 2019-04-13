#include <tuna/priv/windows/embedded_driver.h>

#include <inttypes.h>
#include <stdio.h>

#include <windows.h>
#include <shlwapi.h>

#include <tuna/priv/windows/translate_err_code.h>

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
        err = tuna_translate_err_code(GetLastError());
        goto fail;
    }

    if (!WriteFile(file_handle,
                   embedded_file->data,
                   (DWORD)embedded_file->size,
                   &(DWORD){0},
                   NULL))
    {
        err = tuna_translate_err_code(GetLastError());
        goto fail;
    }

  done:;
    if (file_handle != INVALID_HANDLE_VALUE) { CloseHandle(file_handle); }
    return err;
  fail:;
    if (file_handle != INVALID_HANDLE_VALUE) { DeleteFileA(file_path); }
    goto done;
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
        goto fail;
    }

    char dir_name[MAX_PATH];
    if ((unsigned)snprintf(dir_name, sizeof(dir_name),
                           "TunaDriverFiles-%"PRIu32,
                           (uint32_t)GetCurrentThreadId()) >= sizeof(dir_name))
    {
        err = TUNA_UNEXPECTED;
        goto fail;
    }

    if (!PathAppendA(extruded_driver->dir, dir_name)) {
        err = TUNA_UNEXPECTED;
        goto fail;
    }

    if (!CreateDirectoryA(extruded_driver->dir, NULL)) {
        DWORD err_code = GetLastError();
        if (err_code != ERROR_ALREADY_EXISTS) {
            err = tuna_translate_err_code(err_code);
            goto fail;
        }
    }
    dir_created = 1;

    if ((err = tuna_extrude_file(extruded_driver->inf,
                                 extruded_driver->dir,
                                 embedded_driver->inf)))
    { goto fail; }
    inf_extruded = 1;

    if ((err = tuna_extrude_file(extruded_driver->cat,
                                 extruded_driver->dir,
                                 embedded_driver->cat)))
    { goto fail; }
    cat_extruded = 1;

    if ((err = tuna_extrude_file(extruded_driver->sys,
                                 extruded_driver->dir,
                                 embedded_driver->sys)))
    { goto fail; }
    sys_extruded = 1;

  done:;
    return err;
  fail:;
    if (sys_extruded) { DeleteFileA(extruded_driver->sys); }
    if (cat_extruded) { DeleteFileA(extruded_driver->cat); }
    if (inf_extruded) { DeleteFileA(extruded_driver->inf); }
    if (dir_created) { RemoveDirectoryA(extruded_driver->dir); }
    goto done;
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
    
    tuna_error err = 0;
    
    tuna_extruded_driver extruded_driver;
    if ((err = tuna_extrude_driver(&extruded_driver, embedded_driver))) {
        goto fail;
    }
    extruded = 1;





  done:;
    if (extruded) { tuna_erase_extruded_driver(&extruded_driver); }
    return err;
  fail:;
    goto done;
}
