#include "Server.hpp"
#include <zmq_addon.hpp>

#include <array>
#include <chrono>
#include <iostream>
#include <unistd.h>

Server::Server(std::string_view server_name, int ship_port_num,
               int crew_port_num)
    : info(server_name), context(1),
      ship_router(context, zmq::socket_type::router),
      client_router(context, zmq::socket_type::router),
      core_pull(context, zmq::socket_type::pull),
      ship_dealer(context, zmq::socket_type::dealer),
      control_pub(context, zmq::socket_type::pub) {

  assert(context.handle() != nullptr);
  std::string addr = std::string(server_name) + std::to_string(ship_port_num);
  std::cout << "binding ship router\n";
  ship_router.bind(addr);
  assert(ship_router.handle() != nullptr);
  addr = std::string(server_name) + std::to_string(crew_port_num);
  std::cout << "binding client router\n";
  client_router.bind(addr);
  assert(client_router.handle() != nullptr);
  core_pull.bind("tcp://localhost:*");
  assert(core_pull.handle() != nullptr);
  control_pub.bind("tcp://localhost:*");
  assert(control_pub.handle() != nullptr);

  core_thread = std::thread([this]() { core_task(); });
  user_input_thread = std::thread([this]() { user_input_task(); });
  ship_listener_thread = std::thread([this]() { ship_listener_task(); });
  client_listener_thread = std::thread([this]() { client_listener_task(); });
}

Server::~Server() {
  alive = false;
  zmq::message_t kill(0);
  control_pub.send(kill, zmq::send_flags::none);

  if (user_input_thread.joinable())
    user_input_thread.join();
  std::cout << "joined user input thread\n";
  if (ship_listener_thread.joinable())
    ship_listener_thread.join();
  std::cout << "joined ship_listener thread\n";
  if (client_listener_thread.joinable())
    client_listener_thread.join();
  std::cout << "joined client_listener thread\n";
  if (core_thread.joinable())
    core_thread.join();
  std::cout << "joined core thread\n";
}

bool Server::is_alive() const { return alive; }

void Server::core_task() {
  while (is_alive()) {
    std::vector<zmq::message_t> reqs;
    zmq::recv_result_t res =
        zmq::recv_multipart(core_pull, std::back_inserter(reqs));
    assert(res.has_value() && "recv multipart on core_pull");
    if (reqs.size() < 3)
      continue;
    if (reqs[0].to_string_view() == "CLIENT")
      handle_user_input(reqs);
    else if (reqs[0].to_string_view() == "SHIP")
      handle_sub_ship_input(reqs);
    else if (reqs[1].to_string_view() == "CREW")
      handle_crewmate_input(reqs);
  }
}

void Server::user_input_task() {
  zmq::socket_t core_sender(context, zmq::socket_type::push);
  core_sender.connect(
      core_pull.get(zmq::sockopt::last_endpoint)); // connects to core socket
  assert(core_sender.handle() != nullptr && "user to core handle");
  while (is_alive()) {
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
    } else {
      continue;
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
  zmq::socket_t control_sub(context, zmq::socket_type::sub);
  control_sub.connect(control_pub.get(
      zmq::sockopt::last_endpoint)); // connects to control socket
  control_sub.set(zmq::sockopt::subscribe,
                  ""); // setting the filter, basically all messages

  zmq::pollitem_t items[] = {{ship_router, 0, ZMQ_POLLIN, 0},
                             {control_sub, 0, ZMQ_POLLIN, 0}};
  while (is_alive()) {
    zmq::poll(items, 2, std::chrono::milliseconds(-1)); // indefinite polling
    std::vector<zmq::message_t> reqs;
    if (items[0].revents & ZMQ_POLLIN) {

      zmq::recv_result_t res = zmq::recv_multipart(
          ship_router, std::back_inserter(reqs), zmq::recv_flags::none);
      assert(res.has_value());
      if (reqs[0].to_string_view() == "SHIP") {
        zmq::send_result_t res = zmq::send_multipart(core_sender, reqs);
        assert(res.has_value() && "ship listener to core send");
      }

      if (items[1].revents & ZMQ_POLLIN) {
        std::cout << "ship listener signal to die\n";
        zmq::message_t control_msg(0);
        auto res = control_sub.recv(control_msg);
      }
    }
  }
  std::cout << "ship listener dieing...\n";
}

void Server::client_listener_task() {
  zmq::socket_t core_sender(context, zmq::socket_type::push);
  core_sender.connect(
      core_pull.get(zmq::sockopt::last_endpoint)); // connects to core socket
  assert(core_sender.handle() != nullptr && "client to core handle");
  zmq::socket_t control_sub(context, zmq::socket_type::sub);
  control_sub.connect(control_pub.get(
      zmq::sockopt::last_endpoint)); // connects to control socket
  assert(control_sub.handle() != nullptr && "client to control handle");
  zmq::pollitem_t items[] = {{client_router, 0, ZMQ_POLLIN, 0},
                             {control_sub, 0, ZMQ_POLLIN, 0}};
  control_sub.set(zmq::sockopt::subscribe, "");
  while (is_alive()) {
    std::vector<zmq::message_t> reqs;
    zmq::poll(items, 2, std::chrono::milliseconds(-1)); // indefinite polling
    if (items[0].revents & ZMQ_POLLIN) {
      std::cout << "received msg from crew\n";
      zmq::recv_result_t res = zmq::recv_multipart(
          client_router, std::back_inserter(reqs), zmq::recv_flags::none);

      std::for_each(reqs.begin(), reqs.end(), [](zmq::message_t &req) {
        std::cout << req.to_string() << "\n";
      });

      std::cout << "-----\n";
      assert(res.has_value());
      const pirates::client_id id = reqs[0].to_string();
      const bool have_client = logged_clients.find(id) != logged_clients.end();
      const bool is_logging_in = !have_client &&
                                 reqs[1].to_string_view() == "CREW" &&
                                 reqs[2].to_string_view() == "LOGIN";
      const bool is_correct_format =
          have_client && reqs[1].to_string_view() == "CREW";
      if (is_logging_in || is_correct_format) {
        zmq::send_result_t res = zmq::send_multipart(core_sender, reqs);
        assert(res.has_value() && "crew listener to core send");
      }
    }

    if (items[1].revents & ZMQ_POLLIN) {
      zmq::message_t control_msg(0);
      auto res = control_sub.recv(control_msg);
      std::cout << "client listener task finishing...\n";
    }
  }
  std::cout << "client listener dieing...\n";
}

void Server::handle_user_input(std::span<zmq::message_t> input) {
  assert(input[0].to_string_view() == "CLIENT");
  if (input[1].to_string_view() == "ALERT")
    handle_user_input_alert(input.subspan(2, input.size() - 2));
  else if (input[1].to_string_view() == "COMMAND")
    handle_user_input_command(input.subspan(2, input.size() - 2));
}

void Server::handle_user_input_alert(std::span<zmq::message_t> input) {
  std::cout << "-----------------ALERT------------\n";
  for (auto &msg : input) {
    std::cout << msg.to_string() << "\n";
  }
  std::cout << "-----------------------------\n";
}

void Server::handle_user_input_command(std::span<zmq::message_t> input) {
  std::cout << "--------------COMMAND---------------\n";
  for (auto &msg : input) {
    std::cout << msg.to_string() << "\n";
  }
  std::cout << "-----------------------------\n";
  if (input[0].to_string_view() == "/quit") {
    alive = false;
  }
}

void Server::handle_sub_ship_input(std::span<zmq::message_t> input) {
  std::cout << "-----------------------------\n";
  for (auto &msg : input) {
    std::cout << msg.to_string() << "\n";
  }
  std::cout << "-----------------------------\n";
}

void Server::handle_crewmate_input(std::span<zmq::message_t> input) {
  if (input[2].to_string_view() == "LOGIN")
    handle_crewmate_input_login(input);
  if (input[2].to_string_view() == "COMMAND")
    handle_crewmate_input_command(input.subspan(3, input.size() - 3));
  std::cout << "-----------------------------\n";
  for (auto &msg : input) {
    std::cout << msg.to_string() << "\n";
  }
  std::cout << "-----------------------------\n";
}

void Server::handle_crewmate_input_login(std::span<zmq::message_t> input) {
  pirates::client_id id = input[0].to_string();
  if (logged_clients.contains(id)) // is this possible?
    return;
  std::string username = input[3].to_string();
  std::string password = input[4].to_string();
  std::cout << "New user? " << !saved_crewmembers.contains(username) << "\n";
  auto iter = saved_crewmembers.find(username);
  if (iter == saved_crewmembers.end() ||
      (iter != saved_crewmembers.end() &&
       iter->second.c_password == password)) { // never signed in or old user
    logged_clients[id] = username;
    saved_crewmembers[username] = pirates::client_info(username, password);
    saved_crewmembers[username].c_id = id;
    std::array<zmq::const_buffer, 3> ack = {
        zmq::buffer(id), zmq::str_buffer("SHIP"), zmq::str_buffer("ACK")};
    zmq::send_multipart(client_router, ack);
  }
}

void Server::handle_crewmate_input_command(std::span<zmq::message_t> input) {}

void Server::handle_crewmate_input_text(std::span<zmq::message_t> input) {}
