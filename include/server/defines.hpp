#ifndef DEFINES_HPP
#define DEFINES_HPP

#include <string>
#include <string_view>

namespace pirates {

typedef uint32_t client_id;
typedef uint32_t server_id;
typedef uint32_t game_id;

struct client_info {
  client_id c_id;
  const std::string c_title;

  client_info(std::string_view title);
};

struct server_info {
    server_id s_id;
    const std::string s_title;
    uint32_t server_hops;
    server_info(std::string_view title);

    std::vector<client_info> clients;
    std::vector<game_id> games;
};

struct game_info {
    game_id g_id;
    server_id s_id;
    const std::string title;
    const std::string description;
    uint32_t curr_playing;
};

inline client_id generate_client_id() {
  static client_id curr_id = 0;
  return curr_id++;
}

inline server_id generate_server_id() {
    static server_id curr_id = 0;
    return curr_id++;
}

inline game_id generate_game_id() {
    static game_id curr_id = 0;
    return curr_id++;
}

}; // namespace pirates

#endif
