#ifndef PIRATES_SHIP_DEFINES_HPP
#define PIRATES_SHIP_DEFINES_HPP

#include "zmq_addon.hpp"

#include <string>
#include <string_view>
#include <span>


namespace pirates {

namespace ship {

typedef std::string client_id;
typedef std::string server_id;
typedef std::string cabin_id;

struct client_info {
  client_id c_id;
  std::string c_username;
  std::string c_password;
  bool c_online = true;

  client_info(std::string_view username="", std::string_view password="") { c_username = username; c_password = password; }
  inline void set_id(client_id id) { c_id = id; }
  inline client_id get_id() const { return c_id; }
};

struct cabin_info {
    cabin_id g_id;
    std::string title;
    std::string description;
    uint32_t curr_playing = 0;

    cabin_info(std::string_view t="", std::string_view d="");
};

struct server_info {
    server_id s_id;
    const std::string s_title;
    uint32_t server_hops;

    std::vector<client_info> clients;
    std::vector<cabin_info> games;

    server_info(std::string_view title) : s_title(title) {}
};


void print_multipart_msg(const zmq::multipart_t &mp);
void print_multipart_msg(std::vector<zmq::message_t> &mp);
void print_multipart_msg(std::span<zmq::message_t> &mp);
};

}; // namespace pirates

#endif
