#include <arpa/inet.h>
#include <sys/socket.h>

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "client_session.hpp"

void handle_client(std::shared_ptr<ClientSession> session, std::uint64_t client_id,
                   std::unordered_map<std::uint64_t, std::shared_ptr<ClientSession>>& sessions,
                   std::mutex& sessions_mutex) {
  std::cout << "Клиент подключился" << std::endl;

  while (true) {
    std::string message;
    ReceiveResult result = session->receive(message);

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

    if (!session->send(message)) {
      break;
    }
  }

  {
    std::lock_guard lock(sessions_mutex);
    sessions.erase(client_id);
  }
}

int main() {
  Socket server_socket(socket(AF_INET, SOCK_STREAM, 0));

  if (server_socket.get() == -1) {
    std::perror("socket");
    return 1;
  }

  sockaddr_in address = sockaddr_in();
  address.sin_family = AF_INET;
  address.sin_port = htons(5000);
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  if (bind(server_socket.get(), reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1) {
    std::perror("bind");
    return 1;
  }

  if (listen(server_socket.get(), 1) == -1) {
    std::perror("listen");
    return 1;
  }

  std::vector<std::thread> workers;
  std::unordered_map<std::uint64_t, std::shared_ptr<ClientSession>> sessions;
  std::mutex sessions_mutex;
  std::uint64_t next_id = 1;

  while (true) {
    std::cout << "Ожидание клиента на 127.0.0.1:5000..." << std::endl;

    Socket client_socket(accept(server_socket.get(), nullptr, nullptr));

    if (client_socket.get() == -1) {
      if (errno == EINTR) {
        continue;
      }

      std::perror("accept");
      break;
    }

    const std::uint64_t client_id = next_id++;

    try {
      auto session = std::make_shared<ClientSession>(std::move(client_socket));

      {
        std::lock_guard lock(sessions_mutex);
        sessions.emplace(client_id, session);
      }

      workers.emplace_back(handle_client, session, client_id, std::ref(sessions), std::ref(sessions_mutex));
    } catch (const std::system_error& error) {
      {
        std::lock_guard lock(sessions_mutex);
        sessions.erase(client_id);
      }

      std::cerr << "Не удалось запустить поток: " << error.what() << '\n';
    }
  }

  {
    std::lock_guard lock(sessions_mutex);

    for (const auto& [id, session] : sessions) {
      session->shutdown();
    }
  }

  for (auto& worker : workers) {
    worker.join();
  }

  return 0;
}
