#include "ServerBridgeCabin.hpp"

using namespace pirates::ship;

ServerBridgeCabin::ServerBridgeCabin(std::string_view title,
                                     std::string_view description,
                                     const std::string &shipdeck_enpoint,
                                     const std::string &control_endpoint,
                                     std::string_view server_title)
    : Cabin(title, description, shipdeck_enpoint, control_endpoint),
      bridge_router(get_zmq_context(), zmq::socket_type::router),
      server_name(server_title) {

  std::vector<std::tuple<zmq::socket_t &,
                         std::function<void(std::span<zmq::message_t>)>>>
      sockets_and_handles = {
          {shipdeck_dealer, std::bind(&ServerBridgeCabin::handle_shipdeck_input,
                                      this, std::placeholders::_1)},
          {bridge_router, std::bind(&ServerBridgeCabin::handle_bridge_input,
                                    this, std::placeholders::_1)}};
  poll_with_control(std::move(sockets_and_handles));
}

void ServerBridgeCabin::handle_shipdeck_input(std::span<zmq::message_t> input) {
  std::string_view command = input[0].to_string_view();
  if (command == "CONNECT") {
    handle_shipdeck_input_connect(input);
  } else if (command == "SEND") {
    handle_shipdeck_input_send(input);
  }
  std::string dest_server_title = input[1].to_string();
  if (!next_hops.contains(dest_server_title))
    return;
  server_id next_hop_id = next_hops[dest_server_title];
  std::string sender_type = input[2].to_string();
  std::string m_type = input[3].to_string();
  zmq::multipart_t message;
  message.addstr(next_hop_id);
  message.addstr(dest_server_title);
  message.addstr(server_name);
  message.addstr(sender_type);
  message.addstr(m_type);
  for (auto &msg : input.subspan(4, input.size() - 4)) {
    message.addstr(msg.to_string());
  }
  auto send_res = zmq::send_multipart(bridge_router, message);
  assert(send_res.has_value());
}

void ServerBridgeCabin::handle_bridge_input(std::span<zmq::message_t> input) {}
