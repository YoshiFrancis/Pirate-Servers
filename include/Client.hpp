#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "defines.hpp"

#include <asio.hpp>

class Client {
    using asio::ip::tcp;
    private:
        asio::io_context &io_context;
        std::string name;
        tcp::socket socket;

    public:
        Client(asio::io_context &io);

    private:
        bool connect(std::string_view server_name, std::string_view server_service);
        void disconnect();
        void ask_for_server();
        void handle_write();
        void handle_read();
};

#endif
