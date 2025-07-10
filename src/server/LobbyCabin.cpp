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
  std::cout << "Lobby Cabin got input!\n";
  print_multipart_msg(input);
  std::string_view input_type = input[0].to_string_view();
  if (input_type == "TEXT") {
      std::cout << "lobby cabin sending out text message of users\n";
      std::string username = input[1].to_string();
      std::string_view text = input[2].to_string_view();
      users.insert(username);

      std::array<zmq::const_buffer, 4> send {
          zmq::str_buffer("SEND"),
              zmq::str_buffer("TEXT"),
              zmq::str_buffer("username_placeholder"),
              zmq::buffer(text)
      };

      for (auto& username : users) {
          send[2] = zmq::buffer(username);
          auto send_res = zmq::send_multipart(shipdeck_dealer, send);
          assert(send_res.has_value());
      }


  } else if (input_type == "COMMAND") {

  } else if (input_type == "SHIPDECK") {

  }
}

void LobbyCabin::broadcast_message(std::span<zmq::message_t> input) {}
