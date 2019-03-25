#include <tuna.h>

#include <windows.h>

#include <tap-windows.h>
#include <tap-windows-embedded-files.h>

///////////////////////////////////////////////////////////////////////////////

tuna_error
tuna_create_device(tuna_device **ppDevice)
{
    (void)g_TapWindowsEmbeddedInfFile;
    return 0;
}

void
tuna_destroy_device(tuna_device *pDevice)
{

}


