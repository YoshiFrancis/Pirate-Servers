#include "Server.hpp"

#include <functional>
#include <iostream>

using asio::ip::tcp;

Server::Server(asio::io_context &io, int port_number)
    : io_context(io), acceptor(io, tcp::endpoint(tcp::v4(), port_number)) {
  start_accepting();
}

Server::~Server() { shutdown(); }

void Server::shutdown() {}

void Server::stop_accepting() {}

void Server::start_accepting() {
  ServerConn::conn new_connection = ServerConn::create(io_context);

  acceptor.async_accept(new_connection->socket(),
                        std::bind(&Server::handle_accept, this, new_connection,
                                  asio::placeholders::error));
}

void Server::handle_accept(ServerConn::conn new_connection,
                           const std::error_code &ec) {
  if (!ec) {
    std::cout << "Setting up new connection!\n";
    new_connection->settup();
  } else {
    std::cout << "Error accepting new connection: " << ec.message() << "\n";
  }

  start_accepting();
}

void Server::handle_write() {}

void Server::handle_read() {}
