#ifndef PIRATES_SERVER_LOBBY_CABIN_HPP
#define PIRATES_SERVER_LOBBY_CABIN_HPP
#include "Cabin.hpp"

#include <string>
#include <string_view>
#include <unordered_map>

namespace pirates {

namespace ship {
class LobbyCabin : public Cabin {

private:
  std::unordered_map<client_id, std::string> users;

public:
  LobbyCabin(std::string_view title, std::string_view description,
             const std::string &shipdeck_enpoint,
             const std::string &control_endpoint);

private:
  void handle_shipdeck_input(std::span<zmq::message_t> input);
  void broadcast_message(std::span<zmq::message_t> input);
};
} // namespace ship
} // namespace pirates

#endif
