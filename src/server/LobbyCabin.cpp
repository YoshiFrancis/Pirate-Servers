#include "LobbyCabin.hpp"
#include <iostream>

using namespace pirates::ship;

LobbyCabin::LobbyCabin(std::string_view title, std::string_view description,
                       const std::string &shipdeck_enpoint,
                       const std::string &control_endpoint)
    : Cabin(title, description, shipdeck_enpoint, control_endpoint) {

        std::vector<std::tuple<zmq::socket_t&, std::function<void(std::span<zmq::message_t>)>>> sockets_and_handles = {
            { shipdeck_dealer, std::bind(&LobbyCabin::handle_shipdeck_input, this, std::placeholders::_1) }
        };
        poll_with_control(std::move(sockets_and_handles));

}

void LobbyCabin::handle_shipdeck_input(std::span<zmq::message_t> input) {
  std::string_view input_type = input[0].to_string_view();
  if (input_type == "TEXT") {
      std::string username = input[1].to_string();
      std::string_view text = input[2].to_string_view();

      std::array<zmq::const_buffer, 5> send {
          zmq::str_buffer("SEND"),
              zmq::str_buffer("TEXT"),
              zmq::str_buffer("username_placeholder"),
              zmq::buffer(username),
              zmq::buffer(text)
      };

      for (auto& user : users) {
          send[2] = zmq::buffer(user);
          auto send_res = zmq::send_multipart(shipdeck_dealer, send);
          assert(send_res.has_value());
      }


  } else if (input_type == "COMMAND") {

  } else if (input_type == "SHIPDECK") {

  } else if (input_type == "JOIN") {
      std::string username = input[1].to_string();
      if (!users.contains(username)) {}
          users.insert(username);
  }
}

void LobbyCabin::broadcast_message(std::span<zmq::message_t> input) {}
