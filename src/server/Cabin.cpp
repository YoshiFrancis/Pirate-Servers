#include "Cabin.hpp"

#include <iostream>
#include <string_view>

using namespace pirates::ship;

Cabin::Cabin(std::string_view title, std::string_view description,
             const std::string &shipdeck_enpoint,
             const std::string &control_endpoint)
    : context(1), shipdeck_dealer(context, zmq::socket_type::dealer),
      control_sub(context, zmq::socket_type::sub), info(title, description) {
  assert(context.handle() != nullptr);
  shipdeck_dealer.connect(shipdeck_enpoint);
  assert(shipdeck_dealer.handle() != nullptr);
  control_sub.connect(control_endpoint);
  assert(control_sub.handle() != nullptr);
  control_sub.set(zmq::sockopt::subscribe,
                  ""); // setting the filter, basically all messages
  alive = authenticate_with_server(); // see if connection happens or not
}

Cabin::~Cabin() {}

template <std::ranges::range Range>
zmq::send_result_t Cabin::send_to_shipdeck(Range &&msg) {
  return zmq::send_multipart(shipdeck_dealer, msg);
}

bool Cabin::is_alive() const { return alive; }

std::array<zmq::message_t, 3> Cabin::cabin_info_msg() const {
  return std::array<zmq::message_t, 3>{
      make_zmq_msg(info.title), make_zmq_msg(info.description),
      make_zmq_msg(std::to_string(info.curr_playing))};
}

void Cabin::poll_with_control(
    std::vector<std::tuple<zmq::socket_t &,
                           std::function<void(std::span<zmq::message_t>)>>>
        sockets_and_handles) {

  const int poll_items = sockets_and_handles.size();
  zmq::pollitem_t items[poll_items + 1];
  items[0] = {control_sub, 0, ZMQ_POLLIN, 0};
  for (size_t i = 0; i < poll_items; ++i) {
    items[i+1] = {std::get<0>(sockets_and_handles[i]), 0, ZMQ_POLLIN, 0};
  }
  while (is_alive()) {
    zmq::poll(items, poll_items + 1, std::chrono::milliseconds(-1));
    if (items[0].revents & ZMQ_POLLIN) {
      break;
    }
    std::vector<zmq::message_t> reqs;

    for (size_t i = 0; i < poll_items; ++i) {
        if (items[i+1].revents & ZMQ_POLLIN) {
            auto recv_res = zmq::recv_multipart(std::get<0>(sockets_and_handles[i]), std::back_inserter(reqs));
            assert(recv_res.has_value());
            std::get<1>(sockets_and_handles[i])(reqs); // calling the handle function
            reqs.clear();
        }
    }
  }
}

bool Cabin::authenticate_with_server() {
  std::array<zmq::const_buffer, 2> auth_msg{zmq::str_buffer("REGISTRATION"),
                                            zmq::str_buffer("JOIN")};
  auto send_res = send_to_shipdeck(auth_msg);
  assert(send_res.has_value());
  std::vector<zmq::message_t> reply;
  auto recv_res =
      zmq::recv_multipart(shipdeck_dealer, std::back_inserter(reply));
  assert(recv_res.has_value());
  if (reply[0].to_string_view() == "SHIP" &&
      reply[1].to_string_view() == "SUCCESS") {
    info.g_id = reply[2].to_string();
    send_res = send_to_shipdeck(cabin_info_msg());
    assert(send_res.has_value());
    return true;
  } else {
    return false;
  }
}
