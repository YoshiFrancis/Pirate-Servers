#ifndef SERVER_HPP
#define SERVER_HPP

#include "defines.hpp"
#include "zmq.hpp"

#include <span>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>
#include <atomic>

class Server {

private:
  std::unordered_map<pirates::client_id, std::string> logged_clients;
  std::unordered_map<std::string, pirates::client_info> saved_crewmembers;
  std::unordered_map<pirates::server_id, pirates::server_info> servers;
  std::unordered_map<pirates::game_id, pirates::game_info> owned_games;
  pirates::server_info info;

  std::atomic<bool> alive = true;

  std::thread user_input_thread;
  std::thread ship_listener_thread;
  std::thread client_listener_thread;
  std::thread core_thread;

  zmq::context_t context;

  // socket communicating with all ships following this ship
  zmq::socket_t ship_router;
  // socket communicating with all mates on this ship
  zmq::socket_t client_router;
  // socket communicating from core_thread with the ship, client, and user input
  // threads
  zmq::socket_t core_pull;
  // socket communicating with ship this ship follows (if there is any)
  zmq::socket_t ship_dealer;
  // socket to communicate to finish
  zmq::socket_t control_pub;

public:
  Server(std::string_view server_name, int ship_port_number, int crew_port_num);
  ~Server();

  bool is_alive() const;

private:
  void user_input_task();
  void ship_listener_task();
  void client_listener_task();
  void core_task();

  // Ship can only get commands from user
  void handle_user_input(std::span<zmq::message_t> input);
  void handle_user_input_alert(std::span<zmq::message_t> input);
  void handle_user_input_command(std::span<zmq::message_t> input);

  void handle_sub_ship_input(std::span<zmq::message_t> input);

  void handle_crewmate_input(std::span<zmq::message_t> input);
  void handle_crewmate_input_login(std::span<zmq::message_t> input);
  void handle_crewmate_input_command(std::span<zmq::message_t> input);
  void handle_crewmate_input_text(std::span<zmq::message_t> input);
};

#endif
