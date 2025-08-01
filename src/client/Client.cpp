#include "Client.hpp"
#include "zmq_addon.hpp"
#include <zmq.hpp>

#include <array>
#include <string_view>
#include <thread>
#include <vector>

#include <iostream>

void print_multipart_msg(const zmq::multipart_t &mp) {
  std::cout << mp.size() << " parts:\n";
  for (auto &msg : mp) {
    std::cout << msg.size() << " bytes:\n" << std::endl;
    std::cout << msg.str() << std::endl;
  }
}

void print_multipart_msg(std::vector<zmq::message_t> &mp) {
  std::cout << mp.size() << " parts:\n";
  for (auto &msg : mp) {
    std::cout << msg.size() << " bytes:\n" << std::endl;
    std::cout << msg.str() << std::endl;
  }
}

void print_multipart_msg(std::span<zmq::message_t> &mp) {
  std::cout << mp.size() << " parts:\n";
  for (auto &msg : mp) {
    std::cout << msg.size() << " bytes:\n" << std::endl;
    std::cout << msg.str() << std::endl;
  }
}

Client::Client(std::string_view username_, std::string_view password_,
               std::string_view server_addr_, int port)
    : username(username_), password(password_), server_addr(server_addr_),
      context(1), dealer(context, zmq::socket_type::dealer),
      core(context, zmq::socket_type::pull),
      control_pub(context, zmq::socket_type::pub) {
  // Attempt to connect to server
  dealer.connect(std::string(server_addr));
  // Send first message with username + password
  std::array<zmq::const_buffer, 4> msg = {
      zmq::str_buffer("CREW"), zmq::str_buffer("LOGIN"), zmq::buffer(username),
      zmq::buffer(password)};
  zmq::send_result_t res = zmq::send_multipart(dealer, msg);
  assert(res.has_value());

  while (res == EAGAIN) {
    res = zmq::send_multipart(dealer, msg);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "resending message\n";
  }

  std::vector<zmq::message_t> reqs;
  auto res_result = zmq::recv_multipart_n(dealer, std::back_inserter(reqs), 3);
  assert(res_result.has_value());
  assert(reqs[0].to_string_view() == "SHIP" &&
         reqs[1].to_string_view() == "ACK");
  std::cout << "connected succesfully\n";

  // If successful, start up core socket
  core.bind("tcp://localhost:*"); // use a wild card port
  control_pub.bind("tcp://localhost:*");
  // Begin user input, connection, and core threads
  user_input_thread = std::thread([this]() { input_task(); });
  connection_thread = std::thread([this]() { connection_task(); });
  core_thread = std::thread([this]() { core_task(); });
}

Client::~Client() {
  std::cout << "Client deconstructing\n";
  alive = false;
  zmq::message_t kill(0);
  control_pub.send(kill, zmq::send_flags::none);

  if (user_input_thread.joinable())
    user_input_thread.join();
  std::cout << "joined user input thread\n";
  if (connection_thread.joinable())
    connection_thread.join();
  std::cout << "joined connection thread\n";
  if (core_thread.joinable())
    core_thread.join();
  std::cout << "joined core thread\n";
}

bool Client::is_alive() const { return alive; }

void Client::input_task() {
  zmq::socket_t sender(context, zmq::socket_type::push);
  sender.connect(
      core.get(zmq::sockopt::last_endpoint)); // connects to core socket
  std::unique_lock<std::mutex> lk(input_mux, std::defer_lock);
  while (alive) {
    // get input...
    lk.lock();
    std::string input_str;
    std::cout << "> ";
    std::getline(std::cin, input_str);
    lk.unlock();
    // create message and send
    if (input_str.length() == 0)
      continue;
    std::array<zmq::message_t, 3> client_input = {
        zmq::message_t("CLIENT", 6), zmq::message_t("TEXT", 4),
        zmq::message_t(input_str.data(), input_str.length())};

    if (input_str.length() > 0 && input_str[0] == '/') {
      client_input[1] = zmq::message_t("COMMAND", 7);
    }
    auto send_res = zmq::send_multipart(sender, client_input);
    assert(send_res.has_value());
  }
}

void Client::connection_task() {

  zmq::socket_t sender(context, zmq::socket_type::push);
  sender.connect(
      core.get(zmq::sockopt::last_endpoint)); // connects to core socket
  assert(sender.handle() != nullptr);

  zmq::socket_t control_sub(context, zmq::socket_type::sub);
  control_sub.connect(control_pub.get(
      zmq::sockopt::last_endpoint)); // connects to control socket
  assert(control_sub.handle() != nullptr);
  control_sub.set(zmq::sockopt::subscribe, "");

  zmq::pollitem_t items[2]{{dealer, 0, ZMQ_POLLIN, 0},
                           {control_sub, 0, ZMQ_POLLIN, 0}};

  while (alive) {
    zmq::poll(items, 2,
              std::chrono::milliseconds(-1)); // indefinite polling. (Change?)
    if (items[0].revents & ZMQ_POLLIN) {
      std::vector<zmq::message_t> reqs;
      zmq::recv_result_t res = zmq::recv_multipart(
          dealer, std::back_inserter(reqs), zmq::recv_flags::none);
      assert(res.has_value() && "dealer listener from ship");
      auto send_res = zmq::send_multipart(sender, reqs);
      assert(send_res.has_value() && "dealer thread to core send");
    }

    if (items[1].revents & ZMQ_POLLIN) {
      std::cout << "ship listener signal to die\n";
      zmq::message_t control_msg(0);
      auto res = control_sub.recv(control_msg);
    }
  }
}

void Client::core_task() {
  while (alive) {
    std::vector<zmq::message_t> reqs;
    zmq::recv_result_t res =
        zmq::recv_multipart(core, std::back_inserter(reqs));
    assert(res.has_value() && "Error receiving messagee in core\n");
    if (reqs[0].to_string_view() == "SHIP") {
      handle_ship_input(reqs);
    } else if (reqs[0].to_string_view() == "CABIN") {
      handle_cabin_input(reqs);
    } else if (reqs[0].to_string_view() == "CLIENT") {
      // handle client input
      handle_user_input(reqs);
    }
  }
}

void Client::handle_ship_input(std::span<zmq::message_t> input) {
  if (input[1].to_string_view() == "TEXT") {
    handle_ship_input_text(input);
  } else if (input[1].to_string_view() == "COMMAND") {
    handle_ship_input_command(input);
  } else if (input[1].to_string_view() == "ALERT") {
    handle_ship_input_alert(input);
  } else if (input[1].to_string_view() == "INFO") {
    handle_ship_input_info(input);
  }
}

void Client::handle_ship_input_text(std::span<zmq::message_t> input) {
  for (auto &msg : input.subspan(3, input.size() - 3)) {
    std::cout << input[2].to_string_view() << ": " << msg.to_string_view()
              << "\n";
  }
  std::cout << "< ";
}

void Client::handle_ship_input_command(std::span<zmq::message_t> input) {
  std::string_view command = input[2].to_string_view();

  if (command == "die") {
    alive = false;
  }
}

void Client::handle_ship_input_alert(std::span<zmq::message_t> input) {
  std::cout << "-------------------\n";
  for (auto &msg : input.subspan(2, input.size() - 2)) {
    std::cout << "ALERT: " << msg.to_string_view() << "\n";
  }
  std::cout << "-------------------\n";
  std::cout << "> " << std::flush;
}

void Client::handle_ship_input_info(std::span<zmq::message_t> input) {
  std::cout << "-------------------\n";
  for (auto &msg : input.subspan(2, input.size() - 2)) {
    std::cout << "SHIP INFO: " << msg.to_string_view() << "\n";
  }
  std::cout << "-------------------\n";
  std::cout << "> " << std::flush;
}

void Client::handle_cabin_input(std::span<zmq::message_t> input) {
  std::string_view input_type = input[1].to_string_view();
  if (input_type == "TEXT") {
    // [CABIN, input_type, username of sender, text...]
    std::string_view username = input[2].to_string_view();
    if (input[3].to_string_view() != "") {
      std::cout << username << ": " << input[3].to_string_view() << "\n";
    }
    for (auto &msg : input.subspan(3, input.size() - 4)) {
      if (msg.to_string_view() != "")
        std::cout << "> " << username << ": " << msg.to_string_view() << "\n";
    }

    std::cout << "> " << std::flush;
  }
}

void Client::handle_cabin_input_text(std::span<zmq::message_t> input) {}

void Client::handle_cabin_input_command(std::span<zmq::message_t> input) {}

void Client::handle_cabin_input_alert(std::span<zmq::message_t> input) {}

void Client::handle_user_input(std::span<zmq::message_t> input) {
  if (input[1].to_string_view() == "COMMAND") {
    handle_user_input_command(input);
  } else if (input[1].to_string_view() == "TEXT") {
    handle_user_input_text(input);
  }
}

void Client::handle_user_input_command(std::span<zmq::message_t> input) {
  const std::string command = input[2].to_string();
  if (command == "/quit") {
    std::array<zmq::const_buffer, 3> quit_msg = {zmq::str_buffer("CREW"),
                                                 zmq::str_buffer("COMMAND"),
                                                 zmq::buffer(command)};
    for (size_t i = 0; i < 10; ++i) {
      if (zmq::send_multipart(dealer, quit_msg))
        break;
    }
    alive = false;
  } else if (command.starts_with("/join")) {
    std::array<zmq::const_buffer, 4> join_msg = {
        zmq::str_buffer("CREW"), zmq::str_buffer("COMMAND"),
        zmq::buffer(command.substr(0, 5)), zmq::buffer(command.substr(6))};
    auto send_res = zmq::send_multipart(dealer, join_msg);
    assert(send_res.has_value());
  } else {
    std::array<zmq::const_buffer, 3> command_msg = {zmq::str_buffer("CREW"),
                                                    zmq::str_buffer("COMMAND"),
                                                    zmq::buffer(command)};
    auto send_res = zmq::send_multipart(dealer, command_msg);
    assert(send_res.has_value());
  }
}

void Client::handle_user_input_text(std::span<zmq::message_t> input) {
  std::array<zmq::message_t, 3> send_msgs = {
      zmq::message_t("CREW", 4), zmq::message_t("TEXT", 4),
      zmq::message_t(input[2].data(), input[2].size())};
  // try to send it 10 times
  for (size_t i = 0; i < 10; ++i) {
    auto send_res = zmq::send_multipart(dealer, send_msgs);
    if (send_res.has_value())
      return;
  }
  // reach here, error occured with server.
  alive = false;
}
