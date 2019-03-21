#include <tuna.h>

#include <stddef.h>

#include <windows.h>

#include <tap-windows.h>

///////////////////////////////////////////////////////////////////////////////

extern char const *const pszTapWindowsInfFileName;
extern size_t const cbTapWindowsInfFileSize;
extern unsigned char const *const pTapWindowsInfFileData;

extern char const *const pszTapWindowsCatFileName;
extern size_t const cbTapWindowsCatFileSize;
extern unsigned char const *const pTapWindowsCatFileData;

extern char const *const pszTapWindowsSysFileName;
extern size_t const cbTapWindowsSysFileSize;
extern unsigned char const *const pTapWindowsSysFileData;

tuna_error
tuna_create_device(tuna_device **ppDevice)
{
    return 0;
}

void
tuna_destroy_device(tuna_device *pDevice)
{

}


