#include <tuna.h>

#include <cerrno>

#include <boost/scope_exit.hpp>

#include <thread>
#include <iostream>
#include <iomanip>

int main() {
    std::cout << "included version: "
              << TUNA_MAJOR_VERSION << "." 
              << TUNA_MINOR_VERSION << "." 
              << TUNA_PATCH_VERSION << "\n";
    {
        tuna_version v = tuna_get_version();
        std::cout << "linked version: "
                  << TUNA_GET_MAJOR_VERSION(v) << "." 
                  << TUNA_GET_MINOR_VERSION(v) << "." 
                  << TUNA_GET_PATCH_VERSION(v) << "\n";
    }

    tuna_device *device_a;
    if (auto err = tuna_create_device(TUNA_EXCLUSIVE, &device_a)) {
        std::cerr << "tuna_open_device failed: " << tuna_get_error_name(err) << "\n";
        return EXIT_FAILURE;
    }
    BOOST_SCOPE_EXIT_ALL(&) {
        tuna_free_device(device_a);
    };
    std::cout << "created interface a" << std::flush;

    //char *name_a;
    //if (auto err = tuna_get_name(device_a, &name_a)) {
    //    std::cerr << "tuna_get_name failed: " << tuna_get_error_name(err) << "\n";
    //    return EXIT_FAILURE;
    //}
    //BOOST_SCOPE_EXIT_ALL(&) {
    //    tuna_free_name(name_a);
    //};
    //std::cout << " named " << name_a << std::endl;

    //if (auto err = tuna_set_lifetime(device_a, TUNA_PERSISTENT)) {
    //    std::cerr << "tuna_set_lifetime failed: " << tuna_get_error_name(err) << "\n";
    //    return EXIT_FAILURE;
    //}
    //std::cout << "made " << name_a << " persistent\n";
    //tuna_free_device(device_a); device_a = NULL;

    //if (auto err = tuna_set_status(device_a, TUNA_DISCONNECTED)) {
    //    std::cerr << "tuna_set_status failed: " << tuna_get_error_name(err) << "\n";
    //    return EXIT_FAILURE;
    //}

    //char new_name[] = "footun";
    //if (auto err = tuna_set_name(device_a, new_name)) {
    //    std::cerr << "tuna_set_name failed: " << tuna_get_error_name(err) << "\n";
    //    return EXIT_FAILURE;
    //}
    //std::cout << " renamed to " << new_name << std::endl;

    //if (auto err = tuna_set_mtu(device_a, 2000)) {
    //    std::cerr << "tuna_set_mtu failed: " << tuna_get_error_name(err) << "\n";
    //    return EXIT_FAILURE;
    //}

    //tuna_ip4_address new_address = {
    //    .family = TUNA_IP4,
    //    .value = {20, 30, 40, 50},
    //    .prefix_length = 24,
    //};
    //if (auto e = tuna_add_address(device_a, (tuna_address *)&new_address)) {
    //    std::cerr << "tuna_add_address failed: " << tuna_get_error_name(e) << "\n";
    //    return EXIT_FAILURE;
    //}
    //if (auto e = tuna_remove_address(device_a, (tuna_address *)&new_address)) {
    //    std::cerr << "tuna_remove_address failed: " << tuna_get_error_name(e) << "\n";
    //    return EXIT_FAILURE;
    //}

    //tuna_device *device_b;
    //if (auto err = tuna_attach_device(&device_b, device_a)) {
    //    std::cerr << "tuna_open_device failed: " << tuna_get_error_name(err) << "\n";
    //    return EXIT_FAILURE;
    //}
    //BOOST_SCOPE_EXIT_ALL(&) {
    //    tuna_free_device(device_b);
    //};
    //std::cout << "attached interface b" << std::flush;

    //char *name_b;
    //if (auto err = tuna_get_name(device_b, &name_b)) {
    //    std::cerr << "tuna_get_name failed: " << tuna_get_error_name(err) << "\n";
    //    return EXIT_FAILURE;
    //}
    //BOOST_SCOPE_EXIT_ALL(&) {
    //    tuna_free_name(name_b);
    //};
    //std::cout << " named " << name_b << std::endl;

    //tuna_device_list *devices;
    //if (auto err = tuna_get_device_list(&devices)) {
    //    std::cerr << "tuna_get_device_list failed: " << tuna_get_error_name(err) << "\n";
    //    return EXIT_FAILURE;
    //}
    //BOOST_SCOPE_EXIT_ALL(&) {
    //    tuna_free_device_list(devices);
    //};
    //std::cout << "all devices:\n";
    //for (size_t i = 0; i < tuna_get_device_count(devices); ++i) {
    //    tuna_device const *device = tuna_get_device_at(devices, i);

    //    int index;
    //    if (auto err = tuna_get_index(device, &index)) {
    //        std::cerr << "tuna_get_index failed: " << tuna_get_error_name(err) << "\n";
    //        return EXIT_FAILURE;
    //    }

    //    char *name;
    //    if (auto err = tuna_get_name(device, &name)) {
    //        std::cerr << "tuna_get_name failed: " << tuna_get_error_name(err) << "\n";
    //        return EXIT_FAILURE;
    //    }
    //    BOOST_SCOPE_EXIT_ALL(&) {
    //        tuna_free_name(name);
    //    };
    //    std::cout << "  " << index << ": " << name << "\n";

    //    tuna_ownership ownership;
    //    if (auto err = tuna_get_ownership(device, &ownership)) {
    //        std::cerr << "tuna_get_ownership failed: " << tuna_get_error_name(err) << "\n";
    //        return EXIT_FAILURE;
    //    }
    //    std::cout << "    ownership: " << (ownership ? "shared" : "exclusive") << "\n";

    //    tuna_lifetime lifetime;
    //    if (auto err = tuna_get_lifetime(device, &lifetime)) {
    //        std::cerr << "tuna_get_lifetime failed: " << tuna_get_error_name(err) << "\n";
    //        return EXIT_FAILURE;
    //    }
    //    std::cout << "    lifetime: " << (lifetime ? "persistent" : "transient") << "\n";

    //    tuna_status status;
    //    if (auto err = tuna_get_status(device, &status)) {
    //        std::cerr << "tuna_get_status failed: " << tuna_get_error_name(err) << "\n";
    //        return EXIT_FAILURE;
    //    }
    //    std::cout << "    status: " << (status ? "connected" : "disconnected") << "\n";

    //    size_t mtu;
    //    if (auto err = tuna_get_mtu(device, &mtu)) {
    //        std::cerr << "tuna_get_mtu failed: " << tuna_get_error_name(err) << "\n";
    //        return EXIT_FAILURE;
    //    }
    //    std::cout << "    mtu: " << mtu << "\n";

    //    tuna_address_list *addresses;
    //    if (auto err = tuna_get_address_list(device, &addresses)) {
    //        std::cerr << "tuna_get_address_list failed: " << tuna_get_error_name(err) << "\n";
    //        return EXIT_FAILURE;
    //    }
    //    BOOST_SCOPE_EXIT_ALL(&) {
    //        tuna_free_address_list(addresses);
    //    };
    //    std::cout << "    addresses:\n";
    //    for (size_t j = 0; j < tuna_get_address_count(addresses); ++j) {
    //        auto address = tuna_get_address_at(addresses, j);
    //        switch (address->family) {
    //        case TUNA_IP4:
    //            {
    //                auto *ip4 = (tuna_ip4_address const *)address;
    //                std::cout << "        ip4 "
    //                          << (int)ip4->value[0] << "."
    //                          << (int)ip4->value[1] << "."
    //                          << (int)ip4->value[2] << "."
    //                          << (int)ip4->value[3] << "/"
    //                          << (int)ip4->prefix_length << "\n";
    //            }
    //            break;
    //        case TUNA_IP6:
    //            {
    //                auto *ip6 = (tuna_ip6_address const *)address;
    //                std::cout << "        ip6 "
    //                      << std::hex
    //                      << (int)ip6->value[ 0] << (int)ip6->value[ 1] << ":"
    //                      << (int)ip6->value[ 2] << (int)ip6->value[ 3] << ":"
    //                      << (int)ip6->value[ 4] << (int)ip6->value[ 5] << ":"
    //                      << (int)ip6->value[ 6] << (int)ip6->value[ 7] << ":"
    //                      << (int)ip6->value[ 8] << (int)ip6->value[ 9] << ":"
    //                      << (int)ip6->value[10] << (int)ip6->value[11] << ":"
    //                      << (int)ip6->value[12] << (int)ip6->value[13] << ":"
    //                      << (int)ip6->value[14] << (int)ip6->value[15] << "/"
    //                      << std::dec
    //                      << (int)ip6->prefix_length << "\n";
    //            }
    //            break;
    //        }
    //    }


    //    //if (!strcmp(name, name_a)) {
    //    //    tuna_device *device_x;
    //    //    if (auto err = tuna_attach_device(&device_x, device)) {
    //    //        std::cerr << "tuna_open_device failed: " << tuna_get_error_name(err) << "\n";
    //    //        return EXIT_FAILURE;
    //    //    }
    //    //    BOOST_SCOPE_EXIT_ALL(&) {
    //    //        tuna_free_device(device_x);
    //    //    };
    //    //    std::cout << "attached interface x named " << name << "\n";

    //    //    if (auto err = tuna_set_lifetime(device_x, TUNA_TRANSIENT)) {
    //    //        std::cerr << "tuna_set_lifetime failed: " << tuna_get_error_name(err) << "\n";
    //    //        return EXIT_FAILURE;
    //    //    }
    //    //    std::cout << "made " << name << " transient\n";
    //    //}
    //}
    //std::cout << std::flush;



    //for (int i = 0; i < 100; ++i) {
    //    std::this_thread::sleep_for(std::chrono::seconds(1));

    //    //tuna_status status = (i % 2) ? TUNA_UP : TUNA_DOWN;
    //    //switch (auto e = tuna_set_status(device, status)) {
    //    //  case 0:
    //    //    break;
    //    //  default:
    //    //    std::cerr << "tuna_set_status failed: " << tuna_get_error_name(e) << "\n";
    //    //    return EXIT_FAILURE;
    //    //}
    //    //std::cout << (1 + i) << " status: " << status << std::endl;

    //    std::cout << (1 + i) << std::endl;
    //}
}
