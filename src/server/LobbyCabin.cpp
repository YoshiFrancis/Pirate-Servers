#include "LobbyCabin.hpp"
#include <iostream>

using namespace pirates::ship;

LobbyCabin::LobbyCabin(std::string_view title, std::string_view description,
                       const std::string &shipdeck_enpoint,
                       const std::string &control_endpoint)
    : Cabin(title, description, shipdeck_enpoint, control_endpoint) {

  std::vector<std::tuple<zmq::socket_t &,
                         std::function<void(std::span<zmq::message_t>)>>>
      sockets_and_handles = {
          {shipdeck_dealer, std::bind(&LobbyCabin::handle_shipdeck_input, this,
                                      std::placeholders::_1)}};
  poll_with_control(std::move(sockets_and_handles));
}

void LobbyCabin::handle_shipdeck_input(std::span<zmq::message_t> input) {
  std::string_view input_type = input[0].to_string_view();
  if (input_type == "TEXT") {
    std::string username = input[1].to_string();
    std::string text = input[2].to_string();

    for (auto &user : users) {
      // build on each loop as messages get consumed
      std::array<zmq::message_t, 5> send{
          make_zmq_msg("SEND"), make_zmq_msg("TEXT"),
          make_zmq_msg("username_placeholder"), make_zmq_msg(username),
          zmq::message_t(text.data(), text.size())};
      send[2] = make_zmq_msg(user);
      std::cout << "sending to: " << send[2].to_string_view() << "\n";
      auto send_res = zmq::send_multipart(shipdeck_dealer, send);
      assert(send_res.has_value());
    }

  } else if (input_type == "COMMAND") {

  } else if (input_type == "SHIPDECK") {

  } else if (input_type == "JOIN") {
    std::string username = input[1].to_string();
    if (!users.contains(username)) {
    }
    users.insert(username);
  } else if (input_type == "DISCONNECT") {
    users.erase(input[1].to_string());
  }
}

void LobbyCabin::broadcast_message(std::span<zmq::message_t> input) {}
