#include "Server.hpp"
#include <zmq_addon.hpp>

#include <array>
#include <chrono>
#include <iostream>
#include <unistd.h>

using namespace pirates::ship;

Server::Server(const std::string &shipdeck_addr, std::string_view server_name,
               int ship_port_num, int crew_port_num)
    : info(server_name), context(1),
      ship_router(context, zmq::socket_type::router),
      client_router(context, zmq::socket_type::router),
      shipdeck_dealer(context, zmq::socket_type::dealer),
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
  shipdeck_dealer.connect(shipdeck_addr);
  assert(shipdeck_dealer.handle() != nullptr);
  control_pub.bind("tcp://localhost:*");
  assert(control_pub.handle() != nullptr);

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
}

bool Server::is_alive() const { return alive; }

void Server::user_input_task() {
  while (is_alive()) {
    // get input...
    std::string input_str;
    std::cout << "> ";
    std::getline(std::cin, input_str);
    // create message and send
    std::cout << input_str << "\n";
    if (input_str.length() > 0 && input_str[0] == '/') {
        std::cout << "given a command!\n";
      std::vector<std::string> client_input = {"COMMAND", input_str};
      handle_user_input(client_input);
    }
  }
}

void Server::ship_listener_task() {
  zmq::socket_t control_sub(context, zmq::socket_type::sub);
  control_sub.connect(control_pub.get(
      zmq::sockopt::last_endpoint)); // connects to control socket
  control_sub.set(zmq::sockopt::subscribe,
                  ""); // setting the filter, basically all messages

  zmq::pollitem_t items[3] = {{ship_router, 0, ZMQ_POLLIN, 0},
      {shipdeck_dealer, 0, ZMQ_POLLIN, 0},
      {control_sub, 0, ZMQ_POLLIN, 0}};
  while (is_alive()) {
    zmq::poll(items, 3, std::chrono::milliseconds(-1)); // indefinite polling
    std::vector<zmq::message_t> reqs;
    if (items[0].revents & ZMQ_POLLIN) {

        zmq::recv_result_t res = zmq::recv_multipart(
                ship_router, std::back_inserter(reqs), zmq::recv_flags::none);
        assert(res.has_value());
        if (reqs[0].to_string_view() == "SHIP") {
            zmq::send_result_t res = zmq::send_multipart(shipdeck_dealer, reqs);
            assert(res.has_value() && "ship listener to core send");
        }
    }


    if (items[1].revents & ZMQ_POLLIN) {
        zmq::recv_result_t res = zmq::recv_multipart(
                shipdeck_dealer, std::back_inserter(reqs), zmq::recv_flags::none);
        assert(res.has_value());
        zmq::send_multipart(client_router, reqs);
    }

    if (items[2].revents & ZMQ_POLLIN) {
        std::cout << "ship listener signal to die\n";
        zmq::message_t control_msg(0);
        auto res = control_sub.recv(control_msg);
        break;
    }

  }
  std::cout << "ship listener dieing...\n";
}

void Server::client_listener_task() {
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
      zmq::recv_result_t recv_res = zmq::recv_multipart(
          client_router, std::back_inserter(reqs), zmq::recv_flags::none);
      assert(recv_res.has_value());
      std::cout << "server received message from client\n";
      zmq::send_result_t send_res = zmq::send_multipart(shipdeck_dealer, reqs);
      assert(send_res.has_value() && "crew listener to core send");
    }

    if (items[1].revents & ZMQ_POLLIN) {
      zmq::message_t control_msg(0);
      auto res = control_sub.recv(control_msg);
      std::cout << "client listener task finishing...\n";
    }
  }
  std::cout << "client listener dieing...\n";
}

void Server::handle_user_input(std::span<std::string> input) {
  if (input[0] == "COMMAND")
    handle_user_input_command(input);
}

void Server::handle_user_input_command(std::span<std::string> input) {
  std::cout << "--------------COMMAND---------------\n";
  for (auto &msg : input) {
    std::cout << msg << "\n";
  }
  std::cout << "-----------------------------\n";
  if (input[1] == "/quit") {
    alive = false;
  }
}
