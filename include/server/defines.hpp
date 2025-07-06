#ifndef PIRATES_SHIP_DEFINES_HPP
#define PIRATES_SHIP_DEFINES_HPP

#include <string>
#include <string_view>

namespace pirates {

namespace ship {

typedef std::string client_id;
typedef uint32_t server_id;
typedef uint32_t game_id;

struct client_info {
  client_id c_id;
  std::string c_username;
  std::string c_password;
  bool c_online = true;

  client_info(std::string_view username="", std::string_view password="") { c_username = username; c_password = password; }
  inline void set_id(client_id id) { c_id = id; }
  inline client_id get_id() const { return c_id; }
};

struct server_info {
    server_id s_id;
    const std::string s_title;
    uint32_t server_hops;

    std::vector<client_info> clients;
    std::vector<game_id> games;

    server_info(std::string_view title) : s_title(title) {}
};

struct game_info {
    game_id g_id;
    server_id s_id;
    const std::string title;
    const std::string description;
    uint32_t curr_playing;
};

inline server_id generate_server_id() {
    static server_id curr_id = 0;
    return curr_id++;
}

inline game_id generate_game_id() {
    static game_id curr_id = 0;
    return curr_id++;
}

};

}; // namespace pirates

#endif
