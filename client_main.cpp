#include "client/Client.hpp"

#include <stdexcept>
#include <condition_variable>

#include <cstdlib>

int main(int argc, char* argv[]) { 
    if (argc != 4) {
        std::cout << "Usage: ./MainClient <username> <password> <address>\n";
        return EXIT_FAILURE;
    }
    try {

        std::condition_variable player_alive_cv;

        Client player(argv[1], argv[2], argv[3]);
        while (player.is_alive()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    } catch (std::runtime_error& e) {
        std::cout << "Caught unresolved exception: " << e.what() << "\n";
    } catch(zmq::error_t& e) {
        std::cout << "zmq exception: " << e.what() << "\n";
    }

    return EXIT_SUCCESS; 
}
