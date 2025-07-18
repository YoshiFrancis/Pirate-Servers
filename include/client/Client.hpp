#ifndef PIRATES_CLIENT_HPP
#define PIRATES_CLIENT_HPP

#include "display.hpp"

#include <atomic>
#include <span>
#include <string>
#include <string_view>
#include <thread>

#include "../zhelpers.hpp"

class Client {

private:
  const std::string username;
  const std::string password;
  const std::string server_addr;

  std::mutex input_mux;

  Display display;

  // let threads know when to stop
  std::atomic<bool> alive = true;

  std::thread user_input_thread;
  std::thread connection_thread;
  std::thread core_thread;

  zmq::context_t context;

  // socket to receive input from connection_thread (server input)
  zmq::socket_t dealer;
  // socket to receive input from dealer + user_input threads
  zmq::socket_t core;
  // socket to control lifespan of threads
  zmq::socket_t control_pub;

public:
  Client(std::string_view username_, std::string_view password_,
         std::string_view server_addr_, int port = 5555);
  ~Client();

  bool is_alive() const;

private:
  // works inside the input_thread
  // will handle user events
  void input_task();

  // works inde the connection thread
  // will handle events from the connection socket
  void connection_task();

  // works inside the core thread
  // will handle all input with processing and send to some display
  void core_task();

  void handle_ship_input(std::span<zmq::message_t> input);
  void handle_ship_input_text(std::span<zmq::message_t> input);
  void handle_ship_input_command(std::span<zmq::message_t> input);
  void handle_ship_input_alert(std::span<zmq::message_t> input);
  void handle_ship_input_info(std::span<zmq::message_t> input);

  void handle_cabin_input(std::span<zmq::message_t> input);
  void handle_cabin_input_text(std::span<zmq::message_t> input);
  void handle_cabin_input_command(std::span<zmq::message_t> input);
  void handle_cabin_input_alert(std::span<zmq::message_t> input);

  void handle_user_input(std::span<zmq::message_t> input);
  void handle_user_input_command(std::span<zmq::message_t> input);
  void handle_user_input_text(std::span<zmq::message_t> input);
};

#endif
