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


