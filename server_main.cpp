#include "server/Server.hpp"

#include <string>
#include <iostream>

#include <cstdlib>

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cout << "Usage: ./MainServer <adress> <ship port> <crewmate port>\n";
        return EXIT_FAILURE;
    }
    std::cout << "begining server!\n";
    try {

        pirates::ship::Server myserver(argv[1], std::stoi(argv[2]), std::stoi(argv[3]));
        while (myserver.is_alive()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        };
    } catch(...) {
        std::cout << "\n\nunresolved error caught in main\n\n";
    }

    return EXIT_SUCCESS;
}
