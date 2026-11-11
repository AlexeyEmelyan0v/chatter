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

  // Blocks sending from different threads
  bool send(const std::string& message) {
    std::lock_guard lock(send_mutex);
    return send_message(socket.get(), message);
  }

  // Only one thread may receive
  ReceiveResult receive(std::string& message) {
    return receive_message(socket.get(), message);
  }

  void shutdown() {
    ::shutdown(socket.get(), SHUT_RDWR);
  }
};
