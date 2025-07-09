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
  zmq::socket_t cabins_router;
  zmq::socket_t ships_pair;
  // control socket to kill threads
  zmq::socket_t control_pub;

  std::unordered_map<std::string, cabin_info>
      cabin_map; // cabin title, cabin info
  std::unordered_map<client_id, client_info>
      client_map; // client zmq id, client info

public:
  // need endpoints to all the services
  ShipDeck(std::string_view ship_server_name, int port);
  ~ShipDeck();

  const std::string get_cabins_router_endpoint() const;
  const std::string get_ships_pair_endpoint() const;
  const std::string get_control_pub_endpoint() const;

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
  void handle_crewmate_input_command(std::span<zmq::message_t> input);
  void handle_crewmate_input_text(std::span<zmq::message_t> input);

  void handle_host_user_input(std::span<std::string> input);

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
