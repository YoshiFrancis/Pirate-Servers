#ifndef PIRATES_SHIP_CREWMATE_CONTAINER_HPP
#define PIRATES_SHIP_CREWMATE_CONTAINER_HPP

#include "zmq.hpp"
#include "Crewmember.hpp"
#include "defines.hpp"

#include <string_view>
#include <span>

namespace pirates {

namespace ship {

class CrewmateContainer {

    private:
        zmq::context_t context;
        // socket listening for all requests from ships
        zmq::socket_t router;
        std::unordered_map<client_id, std::string> logged_users; // zmq_id, username
        std::unordered_map<std::string, Crewmember> saved_users; // username, Crewmate data



    public:
        CrewmateContainer(std::string_view endpoint, bool threaded=true);
    private:
        void handle_request(std::span<zmq::message_t> req_);
};


}
}
#endif
