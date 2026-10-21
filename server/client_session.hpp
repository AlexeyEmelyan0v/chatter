#pragma once
#pragma once

#include <mutex>
#include <string>
#include <utility>

#include "../common/protocol.hpp"
#include "../common/socket.hpp"

class ClientSession {
 private:
  Socket socket;
  std::mutex send_mutex;

 public:
  explicit ClientSession(Socket socket): socket(std::move(socket)) {}

  bool send(const std::string& message) {
    std::lock_guard<std::mutex> lock(send_mutex);
    return send_message(socket.get(), message);
  }

  ReceiveResult receive(std::string& message) {
    return receive_message(socket.get(), message);
  }
};
