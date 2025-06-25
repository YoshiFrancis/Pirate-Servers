#ifndef SERVER_HPP
#define SERVER_HPP

#include "defines.hpp"

#include <unordered_map>
#include <vector>

class Server {

private:
  std::unordered_map<pirates::client_id, pirates::client_info> clients;
  std::unordered_map<pirates::server_id, pirates::server_info> servers;
  std::unordered_map<pirates::game_id, pirates::game_info> owned_games;
  pirates::server_info info;

public:
  Server(int port_number);
  ~Server();

private:
};

#endif
