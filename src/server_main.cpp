#include "Server.hpp"

#include "asio.hpp"

#include <iostream>

int main(int argc, char *argv[]) {

  if (argc != 2) {
    std::cout << "Usage: ./server_main PORTNUM\n";
    return 1;
  }

  int port_num = std::atoi(argv[1]);
  if (port_num < 4000) {
    std::cout << "Invlalid port number! It must be greater than 4000\n";
    return 1;
  }
  asio::io_context io_context;
  Server my_server(io_context, port_num);

  return 0;
}
