#ifndef PIRATES_SHIP_SERVERBRIDGECABIN_HPP
#define PIRATES_SHIP_SERVERBRIDGECABIN_HPP

#include "Cabin.hpp"

#include "zmq.hpp"

#include <unordered_map>

namespace pirates {

namespace ship {

class ServerBridgeCabin : Cabin {

private:
  zmq::socket_t bridge_router;
  std::unordered_map<std::string, server_id> next_hops;
  const std::string server_name;

public:
  ServerBridgeCabin(std::string_view title, std::string_view description,
                    const std::string &shipdeck_enpoint,
                    const std::string &control_endpoint,
                    std::string_view server_title);

private:
  void handle_shipdeck_input(std::span<zmq::message_t> input);
  void handle_shipdeck_input_connect(std::span<zmq::message_t> input);
  void handle_shipdeck_input_send(std::span<zmq::message_t> input);
  void handle_bridge_input(std::span<zmq::message_t> input);
  void broadcast_message(std::span<zmq::message_t> input);
};
} // namespace ship
} // namespace pirates

#endif
