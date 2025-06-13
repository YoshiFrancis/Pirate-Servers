#ifndef SERVER_HPP
#define SERVER_HPP

#include "defines.hpp"

#include "asio.hpp"

#include <unordered_map>
#include <vector>

class Server {

private:
  asio::io_context &io_context;
  std::unordered_map<pirates::client_id, pirates::client_info> clients;
  asio::ip::tcp::acceptor acceptor;

public:
  Server(asio::io_context &io, int port_number);
  ~Server();
  void shutdown();
  void stop_accepting();

private:
  void start_accepting();

  void handle_accept(ServerConn::conn new_connection,
                     const std::error_code &ec);

  void handle_write();
  void handle_read();
};

#endif
