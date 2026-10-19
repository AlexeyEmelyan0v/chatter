#pragma once

#include <arpa/inet.h>
#include <sys/socket.h>

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>

constexpr size_t MAX_MESSAGE_SIZE = 1024 * 1024;

enum class ReceiveResult {
  success,     // Получили все запрошенные байты
  closed,      // Соединение завершилось до получения первого байта
  incomplete,  // Получили часть данных, затем соединение завершилось
  error,       // Ошибка recv()
  too_large
};

bool send_all(int socket_fd, const char* data, size_t size) {
  size_t total_sent = 0;

  while (total_sent < size) {
    ssize_t sent = send(socket_fd, data + total_sent, size - total_sent, MSG_NOSIGNAL);

    if (sent == -1) {
      if (errno == EINTR) {
        continue;
      }

      std::perror("send");
      return false;
    }

    if (sent == 0) {
      std::cerr << "Не удалось продолжить отправку\n";
      return false;
    }

    total_sent += static_cast<size_t>(sent);
  }

  return true;
}

bool send_message(int socket_fd, const std::string& message) {
  if (message.size() > MAX_MESSAGE_SIZE) {
    std::cerr << "Сообщение слишком большое\n";
    return false;
  }

  uint32_t network_size = htonl(static_cast<uint32_t>(message.size()));
  static_assert(sizeof(network_size) == 4, "Заголовок длины должен занимать 4 байта");

  if (!send_all(socket_fd, reinterpret_cast<const char*>(&network_size), sizeof(network_size))) {
    return false;
  }

  return send_all(socket_fd, message.data(), message.size());
}

ReceiveResult receive_exact(int socket_fd, char* buffer, size_t size) {
  size_t total_received = 0;

  while (total_received < size) {
    ssize_t received = recv(socket_fd, buffer + total_received, size - total_received, 0);

    if (received == 0) {
      if (total_received == 0) {
        return ReceiveResult::closed;
      }

      return ReceiveResult::incomplete;
    }

    if (received == -1) {
      if (errno == EINTR) {
        continue;
      }

      std::perror("recv");
      return ReceiveResult::error;
    }

    total_received += static_cast<size_t>(received);
  }

  return ReceiveResult::success;
}

ReceiveResult receive_message(int socket_fd, std::string& message) {
  uint32_t network_size = 0;
  ReceiveResult header_result = receive_exact(socket_fd, reinterpret_cast<char*>(&network_size), sizeof(network_size));

  if (header_result != ReceiveResult::success) {
    return header_result;
  }

  uint32_t size = ntohl(network_size);

  if (size > MAX_MESSAGE_SIZE) {
    return ReceiveResult::too_large;
  }

  message.resize(size);
  ReceiveResult body_result = receive_exact(socket_fd, message.data(), size);

  if (body_result == ReceiveResult::closed) {
    return ReceiveResult::incomplete;
  }

  return body_result;
}