#include "Shipdeck.hpp"

#include "zmq_addon.hpp"

#include <algorithm>
#include <iostream>
#include <ranges>
#include <vector>

using namespace pirates::ship;

ShipDeck::ShipDeck(std::string_view ship_server_name, int port)
    : context(1), server_router(context, zmq::socket_type::router),
      cabins_router(context, zmq::socket_type::router),
      ships_pair(context, zmq::socket_type::pair),
      control_pub(context, zmq::socket_type::pub) {

  assert(context.handle() != nullptr);
  std::string addr = std::string(ship_server_name) + std::to_string(port);
  server_router.bind(addr);
  assert(crew_dealer.handle() != nullptr);
  cabins_router.bind("tcp://localhost:*");
  assert(cabins_router.handle() != nullptr);
  ships_pair.bind("tcp://localhost:*");
  assert(ships_dealer.handle() != nullptr);
  control_pub.bind("tcp://localhost:*");
  assert(control_pub.handle() != nullptr);

  server_listener_thread = std::thread([this]() { server_listener_worker(); });
  services_listener_thread =
      std::thread([this]() { services_listener_worker(); });
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

const std::string ShipDeck::get_cabins_router_endpoint() const {
  return cabins_router.get(zmq::sockopt::last_endpoint);
}

const std::string ShipDeck::get_ships_pair_endpoint() const {
  return ships_pair.get(zmq::sockopt::last_endpoint);
}

const std::string ShipDeck::get_control_pub_endpoint() const {
  return control_pub.get(zmq::sockopt::last_endpoint);
}

void ShipDeck::server_listener_worker() {
  zmq::socket_t control_sub(context, zmq::socket_type::sub);
  control_sub.connect(control_pub.get(zmq::sockopt::last_endpoint));
  assert(control_sub.handle() != nullptr);

  zmq::pollitem_t items[] = {{server_router, 0, ZMQ_POLLIN, 0},
                             {control_sub, 0, ZMQ_POLLIN, 0}};

  while (alive) {
    zmq::poll(items, 2, std::chrono::milliseconds(-1));
    std::vector<zmq::message_t> reqs;
    if (items[0].revents & ZMQ_POLLIN) {

      auto recv_res =
          zmq::recv_multipart(server_router, std::back_inserter(reqs));
      assert(recv_res.has_value());
      std::cout << "shipdeck received message from server\n";

      if (reqs[2].to_string_view() == "SHIP")
        handle_top_ship_input(reqs);
      else if (reqs[2].to_string_view() == "SUB")
        handle_sub_ship_input(reqs);
      else if (reqs[2].to_string_view() == "CREW")
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

  zmq::pollitem_t items[] = {{cabins_router, 0, ZMQ_POLLIN, 0},
                             {ships_pair, 0, ZMQ_POLLIN, 0},
                             {control_sub, 0, ZMQ_POLLIN, 0}};

  while (alive) {
    zmq::poll(items, 3, std::chrono::milliseconds(-1));
    std::vector<zmq::message_t> reqs;

    if (items[0].revents & ZMQ_POLLIN) {
      auto recv_res =
          zmq::recv_multipart(cabins_router, std::back_inserter(reqs));
      assert(recv_res.has_value());
      handle_services_cabins_input(reqs);
    }

    if (items[1].revents & ZMQ_POLLIN) {
      auto recv_res = zmq::recv_multipart(ships_pair, std::back_inserter(reqs));
      assert(recv_res.has_value());
      handle_services_ships_input(reqs);
    }

    if (items[2].revents & ZMQ_POLLIN) {
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
    std::vector<std::string> client_input{"NONE", input_str};
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
  // input[0] == cabin_id
  // input[1] == cabin command for shipdeck
  // input[2] == cabin command for user (usually)
  // input[3] == beginning of arguments..
  if (input[1].to_string_view() == "REGISTRATION") {
    if (input[2].to_string_view() == "JOIN" &&
        !cabin_id_to_info.contains(input[0].to_string())) {
      std::array<zmq::const_buffer, 4> reply{
          zmq::buffer(input[0].to_string()), zmq::str_buffer("SHIP"),
          zmq::str_buffer("SUCCESS"), zmq::buffer(input[0].to_string())};

      auto send_res = zmq::send_multipart(cabins_router, reply);
      assert(send_res.has_value());

      zmq::pollitem_t items[1]{{cabins_router, 0, ZMQ_POLLIN, 0}};
      zmq::poll(items, 1, std::chrono::milliseconds(500));
      if (items[0].revents & ZMQ_POLLIN) {
        std::vector<zmq::message_t> cabin_info_reply;
        auto recv_res = zmq::recv_multipart(
            cabins_router, std::back_inserter(cabin_info_reply));
        assert(recv_res.has_value());
        std::string_view g_id = cabin_info_reply[0].to_string_view();
        std::string_view title = cabin_info_reply[1].to_string_view();
        std::string_view desc = cabin_info_reply[2].to_string_view();

        cabin_info new_cabin(title, desc);
        new_cabin.g_id = g_id;
        new_cabin.curr_playing = 0;
        cabin_name_to_id[std::string(title)] = g_id;
        cabin_id_to_info[cabin_id(g_id)] = std::move(new_cabin);

        std::cout << "successfully added a new cabin: " << title << "\n";
        if (default_cabin == "") {
          std::cout << "default cabin configured to " << title << "\n";
          default_cabin = title;
        }
      }
    }
  } else if (input[1].to_string_view() == "SEND") {
    std::string crew_username = input[3].to_string();
    std::string sender_username = input[4].to_string();
    client_id crew_id = crew_name_to_id[crew_username];
    server_id crew_server_id = client_map[crew_id].get_server_id();
    std::string m_type = input[2].to_string();

    std::vector<zmq::const_buffer> messages{
        zmq::buffer(crew_server_id), zmq::buffer(crew_id),
        zmq::str_buffer("CABIN"), zmq::buffer(m_type),
        zmq::buffer(
            sender_username)}; // server_id, client_id, m_type, message...

    for (auto &frame : input.subspan(5, input.size() - 5)) {
      messages.emplace_back(zmq::buffer(frame.to_string()));
    }
    auto send_res = zmq::send_multipart(server_router, messages);
    assert(send_res.has_value());
  }
}

void ShipDeck::handle_services_ships_input(std::span<zmq::message_t> input) {
  // for now, just send it to the user via the server router
  // no need for any processing as of now
  std::vector<zmq::message_t> input_copy(input.size());
  std::ranges::move(input, input_copy.begin());
  auto send_res = zmq::send_multipart(server_router, input_copy);
}

bool ShipDeck::handle_sub_ship_input(std::span<zmq::message_t> input) {
  return false;
}

bool ShipDeck::handle_top_ship_input(std::span<zmq::message_t> input) {
  return false;
}

void ShipDeck::handle_crewmate_input(std::span<zmq::message_t> input) {
  // input = [s_id, c_id, sender_type (CREW), message_type, arguments]

  std::cout << "input from crewmate:\n";
  print_multipart_msg(input);
  if (input[3].to_string_view() == "LOGIN" &&
      !client_map.contains(input[1].to_string())) {
    server_id s_id = input[0].to_string();
    client_id crew_id = input[1].to_string();

    std::string_view username = input[4].to_string_view();
    std::string_view password = input[5].to_string_view();
    client_map[crew_id] = client_info(username, password);
    client_map[crew_id].set_server_id(s_id);
    client_map[crew_id].set_crew_id(crew_id);
    crew_name_to_id[std::string(username)] = crew_id;

    std::array<zmq::const_buffer, 4> ack_reply{
        zmq::buffer(input[0].to_string()), zmq::buffer(crew_id),
        zmq::str_buffer("SHIP"), zmq::str_buffer("ACK")};
    zmq::send_multipart(server_router, ack_reply);
    // SEND TO INITIAL CABIN
    change_player_cabins(crew_id, cabin_name_to_id[default_cabin]);

  } else if (!client_map.contains(client_id(input[1].to_string()))) {
    return;
  } else if (input[3].to_string_view() == "COMMAND")
    handle_crewmate_input_command(input);
  else if (input[3].to_string_view() == "TEXT")
    handle_crewmate_input_text(input);
}

// TODO NEXT
void ShipDeck::handle_crewmate_input_command(std::span<zmq::message_t> input) {
  // input = [s_id, c_id, sender_type (CREW), message_type, command name,
  // command arguments]
  std::string_view command = input[4].to_string_view();
  client_id crew_id = input[1].to_string();
  server_id s_id = input[0].to_string();
  if (command == "/leave") {
    // send them to the initial cabin
    cabin_id default_cabin_id = cabin_name_to_id[default_cabin];
    if (client_map[crew_id].get_cabin_id() != default_cabin_id) {
      change_player_cabins(crew_id, default_cabin_id);
    }
  } else if (command == "/join") {
    std::string cabin_name = input[5].to_string();
    cabin_id cab_id = cabin_name_to_id[cabin_name];
    if (cab_id != client_map[crew_id].get_cabin_id()) {
      change_player_cabins(crew_id, cab_id);
    }

  } else if (command == "/show") {
    cabin_id curr_cabin_id = client_map[crew_id].get_cabin_id();
    if (cabin_id_to_info.contains(curr_cabin_id)) {
      zmq::multipart_t show_info_mp =
          crew_header_mp(s_id, crew_id, "SHIP", "INFO");
      auto cabin_zmq_info_msg = cabin_id_to_info[curr_cabin_id].zmq_info();
      show_info_mp.append(std::move(cabin_zmq_info_msg));
      auto send_res = zmq::send_multipart(server_router, show_info_mp);
      assert(send_res.has_value());
    }
  } else if (command == "/ping") {
    zmq::multipart_t heartbeat_mp =
        crew_header_mp(s_id, crew_id, "SHIP", "ACK");
    auto send_res = zmq::send_multipart(server_router, heartbeat_mp);
    assert(send_res.has_value());
    // send the the rtt from server to user
  } else if (command == "/showall") {
    zmq::multipart_t showall_info_mp =
        crew_header_mp(s_id, crew_id, "SHIP", "INFO");
    std::for_each(cabin_id_to_info.begin(), cabin_id_to_info.end(),
                  [&showall_info_mp](const auto &cabin) {
                    auto cabin_info_msg = cabin.second.zmq_info();
                    showall_info_mp.append(std::move(cabin_info_msg));
                  });
    auto send_res = zmq::send_multipart(server_router, showall_info_mp);
    assert(send_res.has_value());
  } else if (command == "/quit") {
    client_map[crew_id].set_offline();
    change_player_cabins(crew_id, "");
  }
}

void ShipDeck::handle_crewmate_input_text(std::span<zmq::message_t> input) {
  // send text to all the users in the cabin
  // so, send to the cabin and let the cabin generate all the text messages
  // as some people may have configured to not get any text messages
  client_id crew_id = input[1].to_string();
  cabin_id curr_cabin_id = client_map[crew_id].get_cabin_id();
  std::string username = client_map[crew_id].get_username();
  std::string text_input = input[4].to_string();
  std::array<zmq::const_buffer, 4> msg{
      zmq::buffer(curr_cabin_id), zmq::str_buffer("TEXT"),
      zmq::buffer(username), zmq::buffer(text_input)};
  auto send_res = zmq::send_multipart(cabins_router, msg);
  assert(send_res.has_value());
}

void ShipDeck::handle_host_user_input(std::span<std::string> input) {}

bool ShipDeck::add_cabin(const std::string &endpoint) { return false; }

bool ShipDeck::add_sub_ship(const std::string &endpoint) { return false; }

bool ShipDeck::set_top_ship(const std::string &endpoint) { return false; }

void ShipDeck::change_player_cabins(client_id crew_id, cabin_id cab_id) {
    if (client_map[crew_id].c_online && !cabin_id_to_info.contains(cab_id))
        return;
  std::string crew_username = client_map[crew_id].c_username;
  cabin_id to_disconnect = client_map[crew_id].get_cabin_id();
  if (to_disconnect != "") {
    --cabin_id_to_info[to_disconnect].curr_playing;
  }
  std::array<zmq::const_buffer, 3> disconnect_msg{zmq::buffer(to_disconnect),
                                                  zmq::str_buffer("DISCONNECT"),
                                                  zmq::buffer(crew_username)};
  auto send_res = zmq::send_multipart(cabins_router, disconnect_msg);
  assert(send_res.has_value());

  if (client_map[crew_id].c_online && cabin_id_to_info.contains(cab_id)) {
    std::array<zmq::const_buffer, 3> join_msg{zmq::buffer(cab_id),
                                              zmq::str_buffer("JOIN"),
                                              zmq::buffer(crew_username)};
    ++cabin_id_to_info[cab_id].curr_playing;
    send_res = zmq::send_multipart(cabins_router, join_msg);
    assert(send_res.has_value());
    client_map[crew_id].set_cabin_id(cab_id);
  }
}
