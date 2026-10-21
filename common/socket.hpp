#pragma once

#include <unistd.h>

class Socket {
 private:
  int socket_fd = -1;

 public:
  Socket() = default;

  explicit Socket(int socket_fd) noexcept: socket_fd(socket_fd) {}

  Socket(const Socket&) = delete;
  Socket& operator=(const Socket&) = delete;

  ~Socket() {
    if (socket_fd != -1) {
      close(socket_fd);
    }
  }

  Socket(Socket&& other) noexcept: socket_fd(other.socket_fd) {
    other.socket_fd = -1;
  }

  Socket& operator=(Socket&& other) noexcept {
    if (this == &other) {
      return *this;
    }

    if (socket_fd != -1) {
      close(socket_fd);
    }

    socket_fd = other.socket_fd;
    other.socket_fd = -1;
    return *this;
  }

  int get() const noexcept {
    return socket_fd;
  }
};
