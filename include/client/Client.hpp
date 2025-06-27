#ifndef PIRATES_CLIENT_HPP
#define PIRATES_CLIENT_HPP

#include "display.hpp"

#include <string>
#include <string_view>
#include <thread>

#include "../zhelpers.hpp"

class Client {

private:
  const std::string username;
  const std::string password;
  const std::string server_addr;

  Display display;

  std::thread user_input_thread;
  std::thread connection_thread;
  std::thread core_thread;

  zmq::context_t context;

  // socket to receive input from connection_thread (server input)
  zmq::socket_t dealer;
  // socket to receive input from dealer + user_input threads
  zmq::socket_t core;

public:
  Client(std::string_view username_, std::string_view password_,
         std::string_view server_addr_, int port=5555);
  ~Client();

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
};

#endif
