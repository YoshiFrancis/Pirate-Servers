#include "server/Cabin.hpp"
#include "server/CabinContainer.hpp"
#include "server/CrewmateContainer.hpp"
#include "server/Server.hpp"
#include "server/ShipbridgeContainer.hpp"
#include "server/Shipdeck.hpp"

#include <iostream>
#include <string>

#include <cstdlib>

int main(int argc, char *argv[]) {
  if (argc != 4) {
    std::cout << "Usage: ./MainServer <adress> <ship port> <crewmate port>\n";
    return EXIT_FAILURE;
  }
  std::cout << "begining server!\n";
  try {

    std::string shipdeck_endpoint = "tcp://localhost:";
    constexpr int shipdeck_port{6003};

    pirates::ship::ShipDeck shipdeck(shipdeck_endpoint, shipdeck_port);
    pirates::ship::Cabin defaultCabin("Default", "Testing out the cabins",
                                      shipdeck.get_cabins_router_endpoint(),
                                      shipdeck.get_control_pub_endpoint());

    pirates::ship::Server myserver(
        shipdeck_endpoint + std::to_string(shipdeck_port), argv[1],
        std::stoi(argv[2]), std::stoi(argv[3]));

    while (myserver.is_alive()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    };
  } catch (...) {
    std::cout << "\n\nunresolved error caught in main\n\n";
  }

  return EXIT_SUCCESS;
}
