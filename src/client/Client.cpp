#include "Client.hpp"
#include "zmq_addon.hpp"
#include <zmq.hpp>

#include <array>
#include <string_view>
#include <thread>
#include <vector>

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
    // get input...
    std::string input_str;
    std::cout << "> ";
    std::getline(std::cin, input_str);
    // create message and send
    zmq::multipart_t client_input;
    client_input.pushstr("CLIENT");
    if (input_str.length() > 0 && input_str[0] == '/') {
      client_input.pushstr("COMMAND");
    } else {
      client_input.pushstr("TEXT");
    }
    client_input.pushstr(input_str);
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
    if (reqs.size() < 3)
      continue;
    if (reqs[0].str() == "SHIP") {
      // handle server input
      handle_ship_input(reqs[1].str(), reqs[2].str());
    } else if (reqs[0].str() == "CLIENT") {
      // handle client input
      handle_user_input(reqs[1].str(), reqs[2].str());
    }
  }
}

void Client::handle_ship_input(std::string_view input_type,
                               std::string_view input) {}

void Client::handle_user_input(std::string_view input_type,
                               std::string_view input) {
  if (input_type == "COMMAND") {
    handle_user_input_command(input);
  } else if (input_type == "TEXT") {
    handle_user_input_text(input);
  }
}

void Client::handle_user_input_command(std::string_view input) {
  if (input == "/quit") {
    std::array<zmq::const_buffer, 2> quit_msg = {zmq::str_buffer("COMMAND"),
                                                 zmq::str_buffer("quit")};
    for (size_t i = 0; i < 10; ++i) {
      if (zmq::send_multipart(dealer, quit_msg))
        break;
    }
    alive = false;
  }
}

void Client::handle_user_input_text(std::string_view input) {
  assert(input[0] != '/');
  std::array<zmq::const_buffer, 2> send_msgs = {zmq::str_buffer("TEXT"),
                                                zmq::buffer(input)};
  // try to send it 10 times
  for (size_t i = 0; i < 10; ++i) {
    if (zmq::send_multipart(dealer, send_msgs))
      return;
  }
  // reach here, error occured with server.
  alive = false;
}
