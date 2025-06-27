#include "Client.hpp"
#include "zmq_addon.hpp"
#include <zmq.hpp>

Client::Client(std::string_view username_, std::string_view password_,
               std::string_view server_addr_, int port)
    : username(username_), password(password_), server_addr(server_addr_),
      context(1), dealer(context, zmq::socket_type::dealer),
      core(context, zmq::socket_type::pull) {
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
  alive = false;
  user_input_thread.join();
  connection_thread.join();
  core_thread.join();
}

void Client::input_task() {
  zmq::socket_t sender(context, zmq::socket_type::push);
  sender.connect(
      core.get(zmq::sockopt::last_endpoint)); // connects to core socket
  while (alive) {
    // get input
    // send input to core via sender
    zmq::multipart_t client_input;
    // get input...
    // TODO
    client_input.pushstr("CLIENT");
    zmq::send_multipart(core, client_input);
  }
}

void Client::connection_task() {
  zmq::socket_t sender(context, zmq::socket_type::push);
  sender.connect(
      core.get(zmq::sockopt::last_endpoint)); // connects to core socket
  while (alive) {
    std::vector<zmq::message_t> reqs;
    zmq::recv_result_t res = zmq::recv_multipart(
        dealer, std::back_inserter(reqs), zmq::recv_flags::none);
    zmq::send_multipart(core, reqs);
  }
}

void Client::core_task() {
  while (alive) {
    std::vector<zmq::message_t> reqs;
    zmq::recv_result_t res =
        zmq::recv_multipart(core, std::back_inserter(reqs));
    if (reqs[0].str() == "SERVER") {
      // handle server input
    } else if (reqs[0].str() == "CLIENT") {
      // handle client input
      handle_user_input(reqs[1].str());
    }
  }
}

void Client::handle_user_input(std::string_view input) {}
