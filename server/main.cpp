#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <iostream>

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

int main() {
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (server_fd == -1) {
    std::perror("socket");
    return 1;
  }

  sockaddr_in address = sockaddr_in();
  address.sin_family = AF_INET;
  address.sin_port = htons(5000);
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  if (bind(server_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1) {
    std::perror("bind");
    close(server_fd);
    return 1;
  }

  if (listen(server_fd, 1) == -1) {
    std::perror("listen");
    close(server_fd);
    return 1;
  }

  std::cout << "Ожидание клиента на 127.0.0.1:5000...\n";

  int client_fd = accept(server_fd, nullptr, nullptr);

  if (client_fd == -1) {
    std::perror("accept");
    close(server_fd);
    return 1;
  }

  std::cout << "Клиент подключился" << std::endl;

  char buffer[1024];
  int result = 0;

  while (true) {
    ssize_t received = recv(client_fd, buffer, sizeof(buffer), 0);

    if (received == 0) {
      std::cout << "Клиент завершил отправку\n";
      break;
    }

    if (received == -1) {
      if (errno == EINTR) {
        continue;
      }

      std::perror("recv");
      result = 1;
      break;
    }

    std::cout << "Получено: ";
    std::cout.write(buffer, received);
    std::cout << std::endl;

    if (!send_all(client_fd, buffer, static_cast<size_t>(received))) {
      result = 1;
      break;
    }
  }

  close(client_fd);
  close(server_fd);

  return result;
}