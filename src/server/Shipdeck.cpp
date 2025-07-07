#include "Shipdeck.hpp"

#include "zmq_addon.hpp"

#include <iostream>

using namespace pirates::ship;


ShipDeck::ShipDeck(std::string_view ship_server_name, int port, const std::string &cabin_container_endpoint,
        const std::string &crewmate_container_endpoint,
        const std::string &ships_container_endpoint) 
    : context(1), server_router(context, zmq::socket_type::router), cabins_dealer(context, zmq::socket_type::dealer), crew_dealer(context, zmq::socket_type::dealer), ships_dealer(context, zmq::socket_type::dealer), control_pub(context, zmq::socket_type::pub){
        assert(context.handle() != nullptr);
        std::string addr = std::string(ship_server_name) + std::to_string(port);
        server_router.bind(addr);
        assert(server_router.handle() != nullptr);
        cabins_dealer.connect(cabin_container_endpoint);
        assert(cabins_dealer.handle() != nullptr);
        crew_dealer.connect(crewmate_container_endpoint);
        assert(crew_dealer.handle() != nullptr);
        ships_dealer.connect(ships_container_endpoint);
        assert(ships_dealer.handle() != nullptr);
        control_pub.bind("tcp://localhost:*");
        assert(control_pub.handle() != nullptr);

        server_listener_thread = std::thread([this]() { server_listener_worker(); });
        services_listener_thread = std::thread([this]() { services_listener_worker(); });
        users_input_thread = std::thread([this]() { user_input_worker(); });
    }

ShipDeck::~ShipDeck() {
    alive = false;
    zmq::message_t kill(0);
    control_pub.send(kill, zmq::send_flags::none);

    if (users_input_thread.joinable())
        users_input_thread.join();
    if (server_listener_thread.joinable())
        server_listener_thread.join();
    if (services_listener_thread.joinable())
        services_listener_thread.join();
}


void ShipDeck::server_listener_worker() {
    zmq::socket_t control_sub(context, zmq::socket_type::sub);
    control_sub.connect(control_pub.get(zmq::sockopt::last_endpoint));
    assert(control_sub.handle() != nullptr);

    zmq::pollitem_t items[] = {
        {server_router, 0, ZMQ_POLLIN, 0 },
        {control_sub, 0, ZMQ_POLLIN, 0 }
    };

    while (alive) {
        zmq::poll(items, 2, std::chrono::milliseconds(-1));
        std::vector<zmq::message_t> reqs;
        if (items[0].revents & ZMQ_POLLIN) {

            auto recv_res = zmq::recv_multipart(server_router, std::back_inserter(reqs));
            assert(recv_res.has_value());

            if (reqs[1].to_string_view() == "SHIP")
                handle_top_ship_input(reqs);
            else if (reqs[1].to_string_view() == "SUB")
                handle_sub_ship_input(reqs);
            else if (reqs[1].to_string_view() == "CREW")
                handle_crewmate_input(reqs);
        }

        if (items[1].revents & ZMQ_POLLIN) {
            break;
        }
    }

}

void ShipDeck::services_listener_worker() {
    zmq::socket_t control_sub(context, zmq::socket_type::sub);
    control_sub.connect(control_pub.get(zmq::sockopt::last_endpoint));
    assert(control_sub.handle() != nullptr);

    zmq::pollitem_t items[] = {
        {cabins_dealer, 0, ZMQ_POLLIN, 0 },
        {crew_dealer, 0, ZMQ_POLLIN, 0 },
        {ships_dealer, 0, ZMQ_POLLIN, 0 },
        {control_sub, 0, ZMQ_POLLIN, 0 }
    };

    while (alive) {
        zmq::poll(items, 4, std::chrono::milliseconds(-1));
        std::vector<zmq::message_t> reqs;
        if (items[0].revents & ZMQ_POLLIN) {
            auto recv_res = zmq::recv_multipart(cabins_dealer, std::back_inserter(reqs));
            assert(recv_res.has_value());
            handle_services_cabins_input(reqs);
        }

        if (items[1].revents & ZMQ_POLLIN) {
            auto recv_res = zmq::recv_multipart(cabins_dealer, std::back_inserter(reqs));
            assert(recv_res.has_value());
            handle_services_crew_input(reqs);
        }

        if (items[2].revents & ZMQ_POLLIN) {
            auto recv_res = zmq::recv_multipart(cabins_dealer, std::back_inserter(reqs));
            assert(recv_res.has_value());
            handle_services_ships_input(reqs);
        }

        if (items[3].revents & ZMQ_POLLIN) {
            break;
        }
    }
}

void ShipDeck::user_input_worker() {
    while (alive) {
        // get input...
        std::string input_str;
        std::cout << "> ";
        std::getline(std::cin, input_str);
        // create message and send
        std::vector<std::string> client_input {"NONE", input_str};
        if (input_str.length() > 0 && input_str[0] == '/') {
            client_input[0] = "COMMAND";
        } else if (input_str.length() > 0 && input_str.substr(0, 6) == "alert ") {
            client_input[0] = "ALERT";
        } else {
            continue;
        }
        handle_host_user_input(client_input);
    }
}

void ShipDeck::handle_services_cabins_input(std::span<zmq::message_t> input) {

}

void ShipDeck::handle_services_crew_input(std::span<zmq::message_t> input) {

}

void ShipDeck::handle_services_ships_input(std::span<zmq::message_t> input) {

}


bool ShipDeck::handle_sub_ship_input(std::span<zmq::message_t> input) {
    return false;
}

bool ShipDeck::handle_top_ship_input(std::span<zmq::message_t> input) {
    return false;
}

void ShipDeck::handle_crewmate_input(std::span<zmq::message_t> input) {

    if (input[2].to_string_view() == "LOGIN")
        handle_crewmate_input_login(input);
    if (input[2].to_string_view() == "COMMAND")
        handle_crewmate_input_command(input);
    if (input[2].to_string_view() == "TEXT")
        handle_crewmate_input_text(input);
}

void ShipDeck::handle_crewmate_input_login(std::span<zmq::message_t> input) {
    std::string s_id = input[0].to_string();
    client_id c_id = input[1].to_string();
    zmq::send_multipart(crew_dealer, input);
    std::array<zmq::const_buffer, 4> ack = {
        zmq::buffer(s_id), zmq::buffer(c_id), zmq::str_buffer("SHIP"), zmq::str_buffer("ACK")};
    zmq::send_multipart(server_router, ack);
}

void ShipDeck::handle_crewmate_input_command(std::span<zmq::message_t> input) {

}

void ShipDeck::handle_crewmate_input_text(std::span<zmq::message_t> input) {

}

bool ShipDeck::add_cabin(const std::string &endpoint) {
    return false;
}

bool ShipDeck::add_crewmember(client_id id, client_info info){
    return false;
}

bool ShipDeck::add_sub_ship(const std::string &endpoint) {
    return false;
}

bool ShipDeck::set_top_ship(const std::string &endpoint) {
    return false;
}

void ShipDeck::send_shipdeck_info(client_id id) {

}

bool ShipDeck::add_player_to_cabin(client_id id) {
    return false;
}
