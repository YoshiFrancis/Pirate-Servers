#include "Client.hpp"
#include "zmq_addon.hpp"
#include <zmq.hpp>

#include <array>
#include <string_view>
#include <thread>
#include <vector>

#include <iostream>

void print_multipart_msg(const zmq::multipart_t &mp) {
  std::cout << mp.size() << " parts:\n";
  for (auto &msg : mp) {
    std::cout << msg.size() << " bytes:\n" << std::endl;
    std::cout << msg.str() << std::endl;
  }
}

void print_multipart_msg(std::vector<zmq::message_t> &mp) {
  std::cout << mp.size() << " parts:\n";
  for (auto &msg : mp) {
    std::cout << msg.size() << " bytes:\n" << std::endl;
    std::cout << msg.str() << std::endl;
  }
}

Client::Client(std::string_view username_, std::string_view password_,
               std::string_view server_addr_, int port)
    : username(username_), password(password_), server_addr(server_addr_),
      context(1), dealer(context, zmq::socket_type::dealer),
      core(context, zmq::socket_type::pull) {
  // Attempt to connect to server
  dealer.connect(std::string(server_addr));
  // Send first message with username + password
  std::array<zmq::const_buffer, 4> msg = {
      zmq::str_buffer("CREW"), zmq::str_buffer("LOGIN"), zmq::buffer(username),
      zmq::buffer(password)};
  zmq::send_result_t res = zmq::send_multipart(dealer, msg);
  assert(res.has_value());

  while (res == EAGAIN) {
    res = zmq::send_multipart(dealer, msg);
    std::cout << "resending message\n";
  }
  std::cout << "sent message successfully\n";

  std::vector<zmq::message_t> reqs;
  auto res_result = zmq::recv_multipart_n(dealer, std::back_inserter(reqs), 3);
  assert(res_result.has_value());
  print_multipart_msg(reqs);
  assert(reqs[0].to_string_view() == "SHIP" &&
         reqs[1].to_string_view() == "ACK");
  std::cout << "connected succesfully\n";

  // If successful, start up core socket
  core.bind("tcp://localhost:*"); // use a wild card port
  // Begin user input, connection, and core threads
  user_input_thread = std::thread([this]() { input_task(); });
  connection_thread = std::thread([this]() { connection_task(); });
  core_thread = std::thread([this]() { core_task(); });
}

Client::~Client() {
  alive = false;
  user_input_thread.join();
  connection_thread.join();
  core_thread.join();
}

bool Client::is_alive() const { return alive; }

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
    if (input_str.length() == 0)
      continue;
    std::array<zmq::const_buffer, 3> client_input = {zmq::str_buffer("CLIENT"),
                                                     zmq::str_buffer("TEXT"),
                                                     zmq::buffer(input_str)};

    if (input_str.length() > 0 && input_str[0] == '/') {
      client_input[1] = zmq::str_buffer("COMMAND");
    }

    zmq::send_multipart(sender, client_input);
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
    zmq::send_multipart(sender, reqs);
  }
}

void Client::core_task() {
  while (alive) {
    std::vector<zmq::message_t> reqs;
    zmq::recv_result_t res =
        zmq::recv_multipart(core, std::back_inserter(reqs));
    if (reqs[0].to_string_view() == "SHIP") {
      // handle server input
      handle_ship_input(reqs[1].to_string_view(), reqs[2].to_string_view());
    } else if (reqs[0].to_string_view() == "CLIENT") {
      // handle client input
      handle_user_input(reqs[1].to_string_view(), reqs[2].to_string_view());
    }
  }
}

void Client::handle_ship_input(std::string_view input_type,
                               std::string_view input) {
  if (input_type == "TEXT") {
    handle_ship_input_text(input);
  } else if (input_type == "COMMAND") {
    handle_ship_input_command(input);
  }
}

void Client::handle_ship_input_text(std::string_view input) {
  std::cout << "Ship: " << input << '\n';
}

void Client::handle_ship_input_command(std::string_view input) {
  std::cout << "Ship issued the command: " << input << '\n';
}

void Client::handle_user_input(std::string_view input_type,
                               std::string_view input) {
  std::cout << "received input from user\n";
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
    auto send_res = zmq::send_multipart(dealer,

                                        send_msgs);
    if (send_res.has_value())
      return;
  }
  // reach here, error occured with server.
  alive = false;
}
