#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <system_error>
#include <thread>

#include "client_session.hpp"

void handle_client(std::shared_ptr<ClientSession> client) {
  std::cout << "Клиент подключился" << std::endl;

  while (true) {
    std::string message;
    ReceiveResult result = client->receive(message);

    if (result != ReceiveResult::success) {
      if (result == ReceiveResult::closed) {
        std::cout << "Клиент завершил отправку\n";
      } else if (result == ReceiveResult::too_large) {
        std::cerr << "Сообщение слишком большое\n";
      } else if (result == ReceiveResult::incomplete) {
        std::cerr << "Сообщение получено не полностью\n";
      }

      break;
    }

    std::cout << "Получено: " << message << std::endl;

    if (!client->send(message)) {
      break;
    }
  }
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

  while (true) {
    std::cout << "Ожидание клиента на 127.0.0.1:5000..." << std::endl;

    int client_fd = accept(server_fd, nullptr, nullptr);

    if (client_fd == -1) {
      if (errno == EINTR) {
        continue;
      }

      std::perror("accept");
      close(server_fd);
      return 1;
    }

    try {
      auto session = std::make_shared<ClientSession>(Socket(client_fd));
      std::thread t(handle_client, session);
      t.detach();
    } catch (const std::system_error& error) {
      std::cerr << "Не удалось запустить поток: " << error.what() << '\n';
    }
  }

  close(server_fd);
  return 0;
}
