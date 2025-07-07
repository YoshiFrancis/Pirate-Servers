#ifndef PIRATES_SHIP_CREWMATE_CONTAINER_HPP
#define PIRATES_SHIP_CREWMATE_CONTAINER_HPP

#include "zmq.hpp"

#include <string_view>
#include <span>

namespace pirates {

namespace ship {

class CrewmateContainer {

    private:

    public:
        CrewmateContainer(std::string_view endpoint, bool threaded=true);

        // will login users
        bool send_login(std::span<zmq::message_t> input);

    private:

};


}
}
#endif
