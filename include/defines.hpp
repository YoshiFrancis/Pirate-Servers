#ifndef DEFINES_HPP
#define DEFINES_HPP

#include "ServerConn.hpp"

#include <asio.hpp>

#include <string>

namespace pirates {
    using asio::ip::tcp;

    typedef uint32_t client_id;

    enum class client_t {
        server,
        client,
    };

    struct client_info {
        client_id c_id;
        client_t c_type;
        std::string c_title;
        ServerConn c_conn;
    };

    client_id generate_id() {
        static client_id curr_id = 0;
        return curr_id++;
    }
};

#endif
