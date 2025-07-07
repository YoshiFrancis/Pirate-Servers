#ifndef SERVER_CONN_HPP
#define SERVER_CONN_HPP

#include "asio.hpp"

#include <memory>

/*
 * Used to write to and read from the connection that connected to the server via this socket!
 * Does everything asynchronously!
 * Whenever wanting to write, the message is put in a buffer which eventually gets read out.
 * Whenever reading a message, it will put into a buffer provided in the Constructor, usually
 * by the main server.
 */

class ServerConn : public std::enable_shared_from_this<ServerConn> {
    private:
        asio::ip::tcp::socket& socket_;
        std::string message_;
        bool alive_;
    public:
        typedef std::shared_ptr<ServerConn> conn;

        // no public constructor -> must use this factory function
        // everything will be a shared poiniter! HAHAHAHHAH
        static conn create(asio::io_context& io_context);

        void settup();
        void start();
        asio::ip::tcp::socket& socket();
        void force_disconnect();
        bool is_alive() const;

    private:
        ServerConn(asio::io_context& io_context);

        void handle_write(const std::error_code& ec, size_t bytes);
        void handle_read(const std::error_code& ec, size_t bytes);
        void disconnect();
};


#endif
