#include "LobbyCabin.hpp"
#include <iostream>

using namespace pirates::ship;

LobbyCabin::LobbyCabin(std::string_view title, std::string_view description,
                       const std::string &shipdeck_enpoint,
                       const std::string &control_endpoint)
    : Cabin(title, description, shipdeck_enpoint, control_endpoint) {

  poll_with_control(
      {{shipdeck_dealer, std::bind(&LobbyCabin::handle_shipdeck_input, this,
                                   std::placeholders::_1)}});
}

void LobbyCabin::handle_shipdeck_input(std::span<zmq::message_t> input) {
  std::cout << "Lobby Cabin got input!\n";
  print_multipart_msg(input);
}

void LobbyCabin::broadcast_message(std::span<zmq::message_t> input) {}
