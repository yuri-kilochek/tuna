#include <interfaceptor/virtual_interface.hpp>

#include <thread>
#include <iostream>

int main() {
    boost::asio::io_context io_ctx;
    interfaceptor::virtual_interface vi{io_ctx};

    vi.open();
    for (int i = 0; i < 10; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << (1 + i) << std::endl;
    }
    vi.close();
}
