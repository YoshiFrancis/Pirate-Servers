#include "server/Cabin.hpp"
#include "server/CabinContainer.hpp"
#include "server/CrewmateContainer.hpp"
#include "server/Server.hpp"
#include "server/ShipbridgeContainer.hpp"
#include "server/Shipdeck.hpp"
#include "server/LobbyCabin.hpp"

#include <iostream>
#include <string>

#include <cstdlib>

int main(int argc, char *argv[]) {
  if (argc != 4) {
    std::cout << "Usage: ./MainServer <adress> <ship port> <crewmate port>\n";
    return EXIT_FAILURE;
  }
  try {

    std::string shipdeck_endpoint = "tcp://localhost:";
    constexpr int shipdeck_port{6003};

    std::cout << "setting up shiip deck\n";
    pirates::ship::ShipDeck shipdeck(shipdeck_endpoint, shipdeck_port);
    std::cout << "setting up cabins\n";
    std::thread cabins_thread([&shipdeck]() {

            pirates::ship::LobbyCabin defaultCabin("Default", "Testing out the cabins",
                    shipdeck.get_cabins_router_endpoint(),
                    shipdeck.get_control_pub_endpoint());
            });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    std::thread another_cabin_thread([&shipdeck]() {
            pirates::ship::LobbyCabin anotherLobby("Lobby2", "Come join me bruh bruhssss",
                    shipdeck.get_cabins_router_endpoint(), 
                    shipdeck.get_control_pub_endpoint());
            });

  std::cout << "begining server!\n";

    pirates::ship::Server myserver(
        shipdeck_endpoint + std::to_string(shipdeck_port), argv[1],
        std::stoi(argv[2]), std::stoi(argv[3]));

    while (myserver.is_alive()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    };

    std::cout << "server died\n";

    cabins_thread.join();
    another_cabin_thread.join();
  } catch (...) {
    std::cout << "\n\nunresolved error caught in main\n\n";
  }

  return EXIT_SUCCESS;
}
