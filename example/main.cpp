#include <tuna.h>

#include <cerrno>

#include <boost/scope_exit.hpp>

#include <thread>
#include <iostream>

int main() {
    tuna_device_t device;
    switch (auto e = tuna_create(&device); e) {
      case 0:
        break;
      default:
        std::cerr << "tuna_create failed: " << tuna_get_error_name(e)
                  << " : " << errno << " " << strerror(errno) <<"\n";
        return EXIT_FAILURE;
    }
    BOOST_SCOPE_EXIT_ALL(&) {
        switch (auto e = tuna_destroy(&device); e) {
          case 0:
            break;
          default:
            std::cerr << "tuna_destroy failed: " << tuna_get_error_name(e)
                      << " : " << errno << " " << strerror(errno) <<"\n";
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
            std::cerr << "tuna_get_name failed: " << tuna_get_error_name(e)
                      << " : " << errno << " " << strerror(errno) <<"\n";
            return EXIT_FAILURE;
        }
    }
    std::cout << "named " << name << std::endl;

    switch (auto e = tuna_set_status(&device, TUNA_CONNECTED); e) {
      case 0:
        break;
      default:
        std::cerr << "tuna_set_status failed: " << tuna_get_error_name(e)
                  << " : " << errno << " " << strerror(errno) <<"\n";
        return EXIT_FAILURE;
    }

    std::string new_name = "footun";
    size_t new_name_length = new_name.size();
    switch (auto e = tuna_set_name(&device, new_name.data(), &new_name_length); e) {
      case 0:
        break;
      default:
        std::cerr << "tuna_set_name failed: " << tuna_get_error_name(e)
                  << " : " << errno << " " << strerror(errno) <<"\n";
        return EXIT_FAILURE;
    }
    std::cout << "renamed to " << new_name << std::endl;


    uint_least8_t addr[] = {20, 30, 40, 50};
    tuna_set_ip4_address(&device, addr, 24);

    for (int i = 0; i < 1000; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        tuna_status_t status = (i % 2) ? TUNA_CONNECTED : TUNA_DISCONNECTED;
        //switch (auto e = tuna_set_status(&device, status); e) {
        //  case 0:
        //    break;
        //  default:
        //    std::cerr << "tuna_set_status failed: " << tuna_get_error_name(e)
        //              << " : " << errno << " " << strerror(errno) <<"\n";
        //    return EXIT_FAILURE;
        //}

        std::cout << (1 + i) << " status: " << status << std::endl;
    }
}
