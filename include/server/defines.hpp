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
  server_id c_server_id;
  cabin_id curr_cabin;
  std::string c_username;
  std::string c_password;
  bool c_online = true;

  client_info(std::string_view username="", std::string_view password="") { c_username = username; c_password = password; }
  inline void set_server_id(server_id id) { c_server_id = id; }
  inline void set_crew_id(client_id id) { c_id = id; }
  inline void set_cabin_id(cabin_id id) { curr_cabin = id; }
  inline void set_offline() { c_online = false; }
  inline void set_online() { c_online = true; }
  inline client_id get_id() const { return c_id; }
  inline cabin_id get_cabin_id() const { return curr_cabin; }
  inline server_id get_server_id() const { return c_server_id; }
  inline std::string get_username() const { return c_username; }
};

struct cabin_info {
    cabin_id g_id;
    std::string title;
    std::string description;
    uint32_t curr_playing = 0;

    cabin_info(std::string_view t="", std::string_view d="");

    // get a 3 length array in zmq format to send to user
    zmq::multipart_t zmq_info() const;
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

zmq::multipart_t crew_header_mp(const server_id& s_id, const client_id& c_id, const std::string& sender_type, const std::string& message_type);
};

}; // namespace pirates

#endif
