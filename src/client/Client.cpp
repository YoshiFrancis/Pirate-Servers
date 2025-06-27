#include "Client.hpp"
#include "zmq_addon.hpp"
#include <zmq.hpp>

#include <array>

Client::Client(std::string_view username_, std::string_view password_,
               std::string_view server_addr_, int port)
    : username(username_), password(password_), server_addr(server_addr_),
      context(1), dealer(context, zmq::socket_type::dealer),
      core(context, zmq::socket_type::router) {
  // Attempt to connect to server
  dealer.connect(server_addr + ":" + std::to_string(port));
  // Send first message with username + password
  zmq::multipart_t login_msg;
  login_msg.pushstr(password);
  login_msg.pushstr(username);

  zmq::send_result_t res = zmq::send_multipart(dealer, login_msg);
  assert(res.value_or(0) != 0);
  while (res == EAGAIN) {
    res = zmq::send_multipart(dealer, login_msg);
  }
  login_msg.clear();

  zmq::message_t res_msg;
  auto recv_result = dealer.recv(res_msg, zmq::recv_flags::none);
  assert(recv_result != -1);
  assert(res_msg.str() != "Failed");

  // If successful, start up core socket
  core.bind("tcp://127.0.0.1:*"); // use a wild card port
  // Begin user input, connection, and core threads
  user_input_thread = std::thread([this]() { input_task(); });
  connection_thread = std::thread([this]() { connection_task(); });
  core_thread = std::thread([this]() { core_task(); });
}

Client::~Client() {
  // should i have a sub/pub for a kill signal?
  // i can just have a boolean
  user_input_thread.join();
  connection_thread.join();
  core_thread.join();
}

void Client::input_task() {}

void Client::connection_task() {}

void Client::core_task() {}
