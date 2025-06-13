#include "ServerConn.hpp"

ServerConn::conn ServerConn::create(asio::io_context& io_context) 
{
    return conn(new ServerConn(io_context));
}

void ServerConn::settup() {

}

void ServerConn::start() {

}

asio::ip::tcp::socket& ServerConn::socket() {
    return socket_;
}

void ServerConn::force_disconnect() {

}

bool ServerConn::is_alive() const {
    return alive_;
}


void ServerConn::handle_write(const std::error_code& ec, size_t bytes) {

}

void ServerConn::handle_read(const std::error_code& ec, size_t bytes) {

}

void ServerConn::disconnect() {

}
