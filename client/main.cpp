#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <iostream>
#include <string>

#include "../common/protocol.hpp"

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

  if (!send_message(socket_fd, message)) {
    close(socket_fd);
    return 1;
  }

  std::string response;
  ReceiveResult result = receive_message(socket_fd, response);

  if (result != ReceiveResult::success) {
    if (result == ReceiveResult::closed) {
      std::cerr << "Сервер закрыл соединение без ответа\n";
    } else if (result == ReceiveResult::incomplete) {
      std::cerr << "Ответ получен не полностью\n";
    } else if (result == ReceiveResult::too_large) {
      std::cerr << "Ответ превышает допустимый размер\n";
    }

    close(socket_fd);
    return 1;
  }

  std::cout << "Ответ сервера: " << response << '\n';
  close(socket_fd);
  return 0;
}
