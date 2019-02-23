#include <tuna.h>

#include <cerrno>

#include <boost/scope_exit.hpp>

#include <thread>
#include <iostream>
#include <iomanip>

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

    switch (auto e = tuna_set_status(device, TUNA_ENABLED)) {
      case 0:
        break;
      default:
        std::cerr << "tuna_set_status failed: " << tuna_get_error_name(e)
                  << " : " << errno << " " << strerror(errno) <<"\n";
        return EXIT_FAILURE;
    }

    tuna_address new_address;
    new_address.family = TUNA_IP4;
    new_address.ip4.octets[0] = 20;
    new_address.ip4.octets[1] = 30;
    new_address.ip4.octets[2] = 40;
    new_address.ip4.octets[3] = 50;
    new_address.ip4.prefix_length = 24;
    switch (auto e = tuna_add_address(device, &new_address)) {
      case 0:
        break;
      default:
        std::cerr << "tuna_add_address failed: " << tuna_get_error_name(e) << "\n";
        return EXIT_FAILURE;
    }

    tuna_address const* addresses;
    std::size_t address_count;
    switch (auto e = tuna_get_addresses(device, &addresses, &address_count)) {
      case 0:
        break;
      default:
        std::cerr << "tuna_get_addresses failed: " << tuna_get_error_name(e) << "\n";
        return EXIT_FAILURE;
    }

    std::cout << "addresses:\n";
    for (std::size_t i = 0; i < address_count; ++i) {
        auto& a = addresses[i];
        switch (a.family) {
          case TUNA_IP4:
            std::cout << "\tip4 "
                      << (int)a.ip4.octets[0] << "."
                      << (int)a.ip4.octets[1] << "."
                      << (int)a.ip4.octets[2] << "."
                      << (int)a.ip4.octets[3] << "/"
                      << (int)a.ip4.prefix_length << "\n";
            break;
          case TUNA_IP6:
            std::cout << "\tip6 "
                      << std::hex << (int)a.ip6.hextets[0] << std::dec << ":"
                      << std::hex << (int)a.ip6.hextets[1] << std::dec << ":"
                      << std::hex << (int)a.ip6.hextets[2] << std::dec << ":"
                      << std::hex << (int)a.ip6.hextets[3] << std::dec << ":"
                      << std::hex << (int)a.ip6.hextets[4] << std::dec << ":"
                      << std::hex << (int)a.ip6.hextets[5] << std::dec << ":"
                      << std::hex << (int)a.ip6.hextets[6] << std::dec << ":"
                      << std::hex << (int)a.ip6.hextets[7] << std::dec << "/"
                      << (int)a.ip6.prefix_length << "\n";
            switch (auto e = tuna_remove_address(device, &a)) {
              case 0:
                break;
              default:
                std::cerr << "tuna_remove_address failed: " << tuna_get_error_name(e) << "\n";
                return EXIT_FAILURE;
            }
            break;
          default:
            std::cout << "unknown\n";
            break;
        }
    }

    for (int i = 0; i < 10; ++i) {
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
