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

        Server myserver(argv[1], std::stoi(argv[2]), std::stoi(argv[3]));
        while (myserver.is_alive()) {
        };
    } catch(...) {
        std::cout << "unresolved error caught\n";
    }

    return EXIT_SUCCESS;
}
