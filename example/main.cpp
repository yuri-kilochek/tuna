#include <tuna/virtual_device.hpp>

#include <thread>
#include <iostream>

int main() {
    boost::asio::io_context io_ctx;
    tuna::virtual_device vd{io_ctx};

    vd.install();
    for (int i = 0; i < 10; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << (1 + i) << std::endl;
    }
    //vi.uninstall();
}
