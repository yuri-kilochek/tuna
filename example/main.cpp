#include <tuna.h>

#include <boost/scope_exit.hpp>

#include <thread>
#include <iostream>

int main() {
    tuna_device_t device;
    switch (auto e = tuna_create(&device); e) {
      case 0:
          break;
      default:
        std::cerr << "tuna_create failed: " << tuna_get_error_name(e) << "\n";
        return EXIT_FAILURE;
    }
    BOOST_SCOPE_EXIT_ALL(&) {
        switch (auto e = tuna_destroy(&device); e) {
          case 0:
              break;
          default:
            std::cerr << "tuna_destroy failed: " << tuna_get_error_name(e) << "\n";
            break;
        }
    };

    for (int i = 0; i < 1000; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << (1 + i) << std::endl;
    }
}
