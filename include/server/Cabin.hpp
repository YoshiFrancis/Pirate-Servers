#ifndef PIRATES_SHIP_CABIN_HPP
#define PIRATES_SHIP_CABIN_HPP

#include "defines.hpp"
#include "zmq.hpp"

#include <array>
#include <atomic>
#include <functional>
#include <ranges>
#include <span>
#include <tuple>
#include <unordered_map>

namespace pirates {

namespace ship {

class Cabin {

private:
  zmq::context_t context;
  zmq::socket_t control_sub;
  cabin_info info;

  std::atomic<bool> alive = true;

protected:
  zmq::socket_t shipdeck_dealer;

public:
  Cabin(std::string_view title, std::string_view description,
        const std::string &shipdeck_enpoint,
        const std::string &control_endpoint);
  virtual ~Cabin();

protected:
  template <std::ranges::range Range>
  zmq::send_result_t send_to_shipdeck(Range &&msg);
  virtual bool is_alive() const;
  inline virtual zmq::context_t &get_zmq_context() { return context; };
  virtual std::array<zmq::message_t, 3> cabin_info_msg() const;
  virtual void poll_with_control(
      std::vector<std::tuple<zmq::socket_t &,
                             std::function<void(std::span<zmq::message_t>)>>>
          sockets_and_handles) final;

private:
  bool authenticate_with_server();
};

} // namespace ship
} // namespace pirates

#endif
