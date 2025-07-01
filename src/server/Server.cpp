#include "Server.hpp"
#include <zmq_addon.hpp>

#include <array>
#include <iostream>

Server::Server(std::string_view server_name, int ship_port_num,
               int crew_port_num)
    : info(server_name), context(1),
      ship_router(context, zmq::socket_type::router),
      client_router(context, zmq::socket_type::router),
      core_pull(context, zmq::socket_type::pull),
      ship_dealer(context, zmq::socket_type::dealer) {

  assert(context.handle() != nullptr);
  std::string addr = std::string(server_name) + std::to_string(ship_port_num);
  ship_router.bind(addr);
  assert(ship_router);
  addr = std::string(server_name) + std::to_string(crew_port_num);
  client_router.bind(addr);
  assert(client_router);
  core_pull.bind("tcp://localhost:*");
  assert(core_pull);

  core_thread = std::thread([this]() { core_task(); });
  user_input_thread = std::thread([this]() { user_input_task(); });
  ship_listener_thread = std::thread([this]() { ship_listener_task(); });
  client_listener_thread = std::thread([this]() { client_listener_task(); });
}

Server::~Server() {
  alive = false;
  if (user_input_thread.joinable())
    user_input_thread.join();
  if (ship_listener_thread.joinable())
    ship_listener_thread.join();
  if (client_listener_thread.joinable())
    client_listener_thread.join();
  if (core_thread.joinable())
    core_thread.join();
}

void Server::core_task() {
  while (alive) {
    std::vector<zmq::message_t> reqs;
    zmq::recv_result_t res =
        zmq::recv_multipart(core_pull, std::back_inserter(reqs));
    assert(res.has_value() && "recv multipart on core_pull");
    if (reqs.size() < 3)
      continue;
    if (reqs[0].str() == "CLIENT")
      handle_user_input(reqs);
    else if (reqs[0].str() == "SHIP")
      handle_sub_ship_input(reqs);
    else if (reqs[0].str() == "CREW")
      handle_crewmate_input(reqs);
  }
}

void Server::user_input_task() {
  zmq::socket_t core_sender(context, zmq::socket_type::push);
  core_sender.connect(
      core_pull.get(zmq::sockopt::last_endpoint)); // connects to core socket
  assert(core_sender.handle() != nullptr && "user to core handle");
  while (alive) {
    // get input...
    std::string input_str;
    std::cout << "> ";
    std::getline(std::cin, input_str);
    // create message and send
    std::array<zmq::const_buffer, 3> client_input = {zmq::str_buffer("CLIENT"),
                                                     zmq::str_buffer("NONE"),
                                                     zmq::buffer(input_str)};
    if (input_str.length() > 0 && input_str[0] == '/') {
      client_input[1] = zmq::str_buffer("COMMAND");
    } else if (input_str.length() > 0 && input_str.substr(0, 6) == "alert ") {
      client_input[1] = zmq::str_buffer("ALERT");
    }
    zmq::send_result_t res = zmq::send_multipart(core_sender, client_input);
    assert(res.has_value() && "user to core send");
  }
}

void Server::ship_listener_task() {
  zmq::socket_t core_sender(context, zmq::socket_type::push);
  core_sender.connect(
      core_pull.get(zmq::sockopt::last_endpoint)); // connects to core socket
  assert(core_sender.handle() != nullptr && "ship listener to core handle");
  while (alive) {
    std::vector<zmq::message_t> reqs;
    zmq::recv_result_t res = zmq::recv_multipart(
        ship_router, std::back_inserter(reqs), zmq::recv_flags::none);
    assert(res.has_value());
    if (reqs[0].str() == "SHIP") {
      zmq::send_result_t res = zmq::send_multipart(core_sender, reqs);
      assert(res.has_value() && "ship listener to core send");
    }
  }
}

void Server::client_listener_task() {
  zmq::socket_t core_sender(context, zmq::socket_type::push);
  core_sender.connect(
      core_pull.get(zmq::sockopt::last_endpoint)); // connects to core socket
  assert(core_sender.handle() != nullptr && "client to core handle");
  while (alive) {
    std::vector<zmq::message_t> reqs;
    zmq::recv_result_t res = zmq::recv_multipart(
        ship_router, std::back_inserter(reqs), zmq::recv_flags::none);
    assert(res.has_value());
    if (reqs[0].str() == "CREW") {
      zmq::send_result_t res = zmq::send_multipart(core_sender, reqs);
      assert(res.has_value() && "crew listener to core send");
    }
  }
}

void Server::handle_user_input(std::span<zmq::message_t> input) {}

void Server::handle_sub_ship_input(std::span<zmq::message_t> input) {}

void Server::handle_crewmate_input(std::span<zmq::message_t> input) {}
