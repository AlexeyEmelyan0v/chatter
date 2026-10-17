#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <iostream>
#include <string>

int main() {
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (socket_fd == -1) {
    std::perror("socket");
    return 1;
  }

  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_port = htons(5000);
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  if (connect(socket_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1) {
    std::perror("connect");
    close(socket_fd);
    return 1;
  }

  std::cout << "Подключено к серверу 127.0.0.1:5000\n";

  std::cout << "Введите сообщение(максимум 1024 символа): ";
  std::string message;
  std::getline(std::cin, message);

  if (message.empty()) {
    close(socket_fd);
    return 0;
  }

  ssize_t sent = send(socket_fd, message.data(), message.size(), MSG_NOSIGNAL);

  if (sent == -1) {
    std::perror("send");
    close(socket_fd);
    return 1;
  }

  if (static_cast<size_t>(sent) != message.size()) {
    std::cerr << "Отправлена только часть сообщения\n";
    close(socket_fd);
    return 1;
  }

  char buffer[1024];
  ssize_t received = recv(socket_fd, buffer, sizeof(buffer), 0);

  if (received == -1) {
    std::perror("recv");
    close(socket_fd);
    return 1;
  }

  if (received > 0) {
    std::cout << "Ответ сервера: ";
    std::cout.write(buffer, received);
    std::cout << '\n';
  } else {
    std::cout << "Сервер закрыл соединение\n";
  }

  close(socket_fd);
  return 0;
}