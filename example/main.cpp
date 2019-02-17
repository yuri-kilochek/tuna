#include <tuna.h>

#include <cerrno>

#include <boost/scope_exit.hpp>

#include <thread>
#include <iostream>

int main() {
    tuna_device* device;
    switch (auto e = tuna_create_device(&device)) {
      case 0:
        break;
      default:
        std::cerr << "tuna_create_device failed: " << tuna_get_error_name(e) << "\n";
        return EXIT_FAILURE;
    }
    BOOST_SCOPE_EXIT_ALL(&) {
        tuna_destroy_device(device);
    };
    std::cout << "created interface " << std::flush;

    char const *name;
    switch (auto e = tuna_get_name(device, &name)) {
      case 0:
        break;
      default:
        std::cerr << "tuna_get_name failed: " << tuna_get_error_name(e) << "\n";
        return EXIT_FAILURE;
    }
    std::cout << "named " << name << std::endl;

    std::string new_name = "footun";
    switch (auto e = tuna_set_name(device, new_name.data())) {
      case 0:
        break;
      default:
        std::cerr << "tuna_set_name failed: " << tuna_get_error_name(e) << "\n";
        return EXIT_FAILURE;
    }
    std::cout << "renamed to " << new_name << std::endl;

    std::size_t mtu;
    switch (auto e = tuna_get_mtu(device, &mtu)) {
      case 0:
        break;
      default:
        std::cerr << "tuna_get_mtu failed: " << tuna_get_error_name(e) << "\n";
        return EXIT_FAILURE;
    }
    std::cout << "mtu is " << mtu << std::endl;

    std::size_t new_mtu = 2000;
    switch (auto e = tuna_set_mtu(device, new_mtu)) {
      case 0:
        break;
      default:
        std::cerr << "tuna_set_mtu failed: " << tuna_get_error_name(e) << "\n";
        return EXIT_FAILURE;
    }
    std::cout << "changed mtu to " << new_mtu << std::endl;

    //uint_least8_t addr[] = {20, 30, 40, 50};
    //switch (auto e = tuna_set_ip4_address(&device, addr, 24)) {
    //  case 0:
    //    break;
    //  default:
    //    std::cerr << "tuna_set_ip4_address failed: " << tuna_get_error_name(e)
    //              << " : " << errno << " " << strerror(errno) <<"\n";
    //    return EXIT_FAILURE;
    //}

    for (int i = 0; i < 1000; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        tuna_status status = (i % 2) ? TUNA_ENABLED : TUNA_DISABLED;
        switch (auto e = tuna_set_status(device, status)) {
          case 0:
            break;
          default:
            std::cerr << "tuna_set_status failed: " << tuna_get_error_name(e)
                      << " : " << errno << " " << strerror(errno) <<"\n";
            return EXIT_FAILURE;
        }

        std::cout << (1 + i) << " status: " << status << std::endl;
    }
}
