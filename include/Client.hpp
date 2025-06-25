#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "defines.hpp"

class Client {
private:
  std::string name;

public:
private:
  bool connect(std::string_view server_name, std::string_view server_service);
  void disconnect();
  void ask_for_server();
  void handle_write();
  void handle_read();
};

#endif
