#ifndef PIRATES_SHIP_SHIPDECK_HPP
#define PIRATES_SHIP_SHIPDECK_HPP

#include "defines.hpp"
#include "zmq.hpp"

#include "CabinContainer.hpp"
#include "Crewmember.hpp"
#include "ShipbridgeContainer.hpp"
#include "Shipbridge.hpp"

#include <span>
#include <unordered_map>
#include <vector>

namespace pirates {

namespace ship {

class ShipDeck {
private:
  CabinContainer cabins;
  std::unordered_map<client_id, Crewmember> crewmembers;
  ShipbridgeContainer sub_ships;
  Shipbridge top_ship;

public:
  // one should initialize all the cabins when first starting up the ship deck
  ShipDeck(CabinContainer cabins);
  bool add_cabin(const std::string &endpoint);
  bool add_crewmember(client_id id, client_info info);
  bool add_sub_ship(const std::string &endpoint);
  bool set_top_ship(const std::string &endpoint);

  void input(std::span<zmq::message_t> input);

private:
  // describe all the cabins and the players inside
  void send_shipdeck_info(client_id id);

  bool add_player_to_cabin(client_id id);

  bool handle_sub_ship_input(std::span<zmq::message_t> input);
  bool handle_top_ship_input(std::span<zmq::message_t> input);
  bool handle_crewmember_input(std::span<zmq::message_t> input);
  bool handle_host_user_input(std::span<zmq::message_t> input);
};
} // namespace ship
} // namespace pirates

#endif
