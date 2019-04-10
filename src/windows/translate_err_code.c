#include <tuna/priv/windows/translate_err_code.h>

#include <WinError.h>

tuna_error
tuna_translate_err_code(DWORD err_code) {
    switch (err_code) {
      case 0:;
        return 0;
      default:;
        return TUNA_UNEXPECTED;
    }
}
