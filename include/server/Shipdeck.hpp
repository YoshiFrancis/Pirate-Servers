#ifndef PIRATES_SHIP_SHIPDECK_HPP
#define PIRATES_SHIP_SHIPDECK_HPP

#include "defines.hpp"
#include "zmq.hpp"

#include <span>
#include <thread>
#include <unordered_map>
#include <vector>

namespace pirates {

namespace ship {

class ShipDeck {
private:
  bool alive = true;

  std::thread server_listener_thread;
  std::thread services_listener_thread;
  std::thread users_input_thread;

  zmq::context_t context;

  // socket to communicate with server
  // using a router socket means one ship deck but multiple servers
  zmq::socket_t server_router;
  // sockets to communicate with services
  zmq::socket_t cabins_dealer;
  zmq::socket_t crew_dealer;
  zmq::socket_t ships_dealer;
  // control socket to kill threads
  zmq::socket_t control_pub;

public:
  // need endpoints to all the services
  ShipDeck(std::string_view ship_server_name, int port,
           const std::string &cabin_container_endpoint,
           const std::string &crewmate_container_endpoint,
           const std::string &ships_container_endpoint);
  ~ShipDeck();

private:
  void server_listener_worker();
  void services_listener_worker();
  void user_input_worker();

  void handle_services_cabins_input(std::span<zmq::message_t> input);
  void handle_services_crew_input(std::span<zmq::message_t> input);
  void handle_services_ships_input(std::span<zmq::message_t> input);

  bool handle_sub_ship_input(std::span<zmq::message_t> input);
  bool handle_top_ship_input(std::span<zmq::message_t> input);

  void handle_crewmate_input(std::span<zmq::message_t> input);
  void handle_crewmate_input_login(std::span<zmq::message_t> input);
  void handle_crewmate_input_command(std::span<zmq::message_t> input);
  void handle_crewmate_input_text(std::span<zmq::message_t> input);

  bool handle_host_user_input(std::span<std::string> input);

  bool add_cabin(const std::string &endpoint);
  bool add_sub_ship(const std::string &endpoint);
  bool set_top_ship(const std::string &endpoint);
  // describe all the cabins and the players inside
  void send_shipdeck_info(client_id id);
  bool add_player_to_cabin(client_id id);
};
} // namespace ship
} // namespace pirates

#endif
