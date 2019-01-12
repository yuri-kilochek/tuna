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
    std::cout << "created interface " << std::flush;

    std::string name;
    name.resize(name.capacity());
    retry_get_name: {
        std::size_t length = name.size();
        switch (auto e = tuna_get_name(&device, name.data(), &length); e) {
          case 0:
          case TUNA_NAME_TOO_LONG:
            name.resize(length);
            if (!e) { break; }
            goto retry_get_name;
          default:
            std::cerr << "tuna_get_name failed: " << tuna_get_error_name(e) << "\n";
            return EXIT_FAILURE;
        }
    }
    std::cout << "named " << name << std::endl;

    for (int i = 0; i < 1000; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        tuna_status_t status;
        switch (auto e = tuna_get_status(&device, &status); e) {
          case 0:
            break;
          default:
            std::cerr << "tuna_get_status failed: " << tuna_get_error_name(e) << "\n";
            return EXIT_FAILURE;
        }

        std::cout << (1 + i) << " status: " << status << std::endl;
    }
}
