#ifndef PIRATES_SHIP_CABIN_HPP
#define PIRATES_SHIP_CABIN_HPP

#include "defines.hpp"
#include "zmq.hpp"

#include <unordered_map>
#include <span>
#include <functional>

namespace pirates {

namespace ship {

class Cabin {

    private:
        zmq::context_t context;
        zmq::socket_t shipdeck_dealer;
        zmq::socket_t control_sub;
        cabin_info info;

    public:
        Cabin(std::string_view title, std::string_view description, const std::string& shipdeck_enpoint, const std::string& control_endpoint);
        virtual ~Cabin();

    protected:
        virtual zmq::send_result_t send_to_shipdeck(std::vector<zmq::message_t>&& msg) final;
        virtual void poll_with_control(std::vector<std::tuple<zmq::socket_t&, std::function<void(std::span<zmq::message_t>)>>> sockets_and_handles) final;
};

} // namespace ship
} // namespace pirates

#endif
