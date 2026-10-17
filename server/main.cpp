#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <iostream>

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
  ssize_t received = recv(client_fd, buffer, sizeof(buffer), 0);
  int result = 0;

  if (received > 0) {
    std::cout << "Получено: ";
    std::cout.write(buffer, received);
    std::cout << std::endl;

    ssize_t sent = send(client_fd, buffer, received, MSG_NOSIGNAL);

    if (sent == -1) {
      std::perror("send");
      result = 1;
    } else if (sent != received) {
      std::cout << "Отправлена часть данных\n";
      result = 1;
    }
  } else if (received == 0) {
    std::cout << "Клиент завершил отправку\n";
  } else {
    std::perror("recv");
    result = 1;
  }

  close(client_fd);
  close(server_fd);

  return result;
}