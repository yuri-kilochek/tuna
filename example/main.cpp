#include <tuna/virtual_device.hpp>
#include <boost/asio/use_future.hpp>

#include <thread>
#include <iostream>

int main() {
    boost::asio::io_context io_ctx;
    tuna::virtual_device vd{io_ctx};

    vd.async_install(boost::asio::use_future).get();
    io_ctx.run();
    for (int i = 0; i < 10; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << (1 + i) << std::endl;
    }
    std::cout << "is_installed:" << vd.is_installed() << std::endl;
    vd.uninstall();
    for (int i = 10; i > 0; --i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << i << std::endl;
    }
}
