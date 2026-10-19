#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <iostream>
#include <string>

constexpr size_t BUFFER_SIZE = 1024;

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

constexpr size_t MAX_MESSAGE_SIZE = 1024 * 1024;

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

enum class ReceiveResult {
  success,     // Получили все запрошенные байты
  closed,      // Соединение завершилось до получения первого байта
  incomplete,  // Получили часть данных, затем соединение завершилось
  error,       // Ошибка recv()
  too_large
};

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

int main() {
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (socket_fd == -1) {
    std::perror("socket");
    return 1;
  }

  sockaddr_in address = sockaddr_in();
  address.sin_family = AF_INET;
  address.sin_port = htons(5000);
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  if (connect(socket_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1) {
    std::perror("connect");
    close(socket_fd);
    return 1;
  }

  std::cout << "Подключено к серверу 127.0.0.1:5000\n";

  std::cout << "Введите сообщение: ";
  std::string message;
  std::getline(std::cin, message);

  if (message.empty()) {
    close(socket_fd);
    return 0;
  }

  char buffer[BUFFER_SIZE];
  size_t offset = 0;
  int result = 0;

  while (offset < message.size()) {
    size_t chunk_size = std::min(BUFFER_SIZE, message.size() - offset);

    if (!send_all(socket_fd, message.data() + offset, chunk_size)) {
      result = 1;
      break;
    }

    size_t total_received = 0;

    while (total_received < chunk_size) {
      ssize_t received = recv(socket_fd, buffer, chunk_size - total_received, 0);

      if (received == -1) {
        if (errno == EINTR) {
          continue;
        }

        std::perror("recv");
        result = 1;
        break;
      }

      if (received == 0) {
        std::cerr << "\nСервер закрыл соединение до полного ответа\n";
        result = 1;
        break;
      }

      std::cout << "Получено: ";
      std::cout.write(buffer, received);
      std::cout << std::endl;

      total_received += static_cast<size_t>(received);
    }

    if (result != 0) {
      break;
    }

    offset += chunk_size;
  }

  close(socket_fd);
  return result;
}
