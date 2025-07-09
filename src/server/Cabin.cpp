#include "Cabin.hpp"

#include <string_view>

using namespace pirates::ship;

Cabin::Cabin(std::string_view title, std::string_view description, const std::string& shipdeck_enpoint, const std::string& control_endpoint) : context(1), shipdeck_dealer(context, zmq::socket_type::dealer), control_sub(context, zmq::socket_type::sub), info(title, description) {
    assert(context.handle() != nullptr);
    shipdeck_dealer.connect(shipdeck_enpoint);
    assert(shipdeck_dealer.handle() != nullptr);
    control_sub.connect(control_endpoint);
    assert(control_sub.handle() != nullptr);
    control_sub.set(zmq::sockopt::subscribe,
            ""); // setting the filter, basically all messages
}

zmq::send_result_t Cabin::send_to_shipdeck(std::vector<zmq::message_t>&& msg) {
    return zmq::send_multipart(shipdeck_dealer, msg);
}


void Cabin::poll_with_control(std::vector<std::tuple<zmq::socket_t&, std::function<void(std::span<zmq::message_t>)>>> sockets_and_handles) {

    const int poll_items = sockets_and_handles.size();
    zmq::pollitem_t items[poll_items+1];
    items[0] = { control_sub, 0, ZMQ_POLLIN, 0 };
    for (size_t i = 0; i < poll_items; ++i) {
        items[i] = { std::get<0>(sockets_and_handles[i]), 0, ZMQ_POLLIN, 0 };
    }
    while (true) {
        zmq::poll(items, poll_items + 1, std::chrono::milliseconds(-1));
        if (items[0].revents & ZMQ_POLLIN) {
            break;
        }
    }


}
