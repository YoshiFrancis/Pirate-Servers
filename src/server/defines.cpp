#include "defines.hpp"

#include <iostream>

void pirates::ship::print_multipart_msg(const zmq::multipart_t &mp) {
  std::cout << mp.size() << " parts:\n";
  for (auto &msg : mp) {
    std::cout << msg.size() << " bytes:\n" << std::endl;
    std::cout << msg.str() << std::endl;
  }
}

void pirates::ship::print_multipart_msg(std::vector<zmq::message_t> &mp) {
  std::cout << mp.size() << " parts:\n";
  for (auto &msg : mp) {
    std::cout << msg.size() << " bytes:\n" << std::endl;
    std::cout << msg.str() << std::endl;
  }
}

void pirates::ship::print_multipart_msg(std::span<zmq::message_t> &mp) {
  std::cout << mp.size() << " parts:\n";
  for (auto &msg : mp) {
    std::cout << msg.size() << " bytes:\n" << std::endl;
    std::cout << msg.str() << std::endl;
  }
}

pirates::ship::cabin_info::cabin_info(std::string_view t, std::string_view d) : title(t), description(d) { }

zmq::multipart_t pirates::ship::cabin_info::zmq_info() const {
    zmq::multipart_t info_mp;
    info_mp.addstr("Cabin name: " + title);
    info_mp.addstr("Cabin description: " + description);
    info_mp.addstr("Current playing: " + std::to_string(curr_playing));
    return info_mp;
}

zmq::multipart_t pirates::ship::crew_header_mp(const server_id& s_id, const client_id& c_id, const std::string& sender_type, const std::string& message_type) {
    zmq::multipart_t header_mp;
    header_mp.addstr(s_id);
    header_mp.addstr(c_id);
    header_mp.addstr(sender_type);
    header_mp.addstr(message_type);
    return header_mp;
}
