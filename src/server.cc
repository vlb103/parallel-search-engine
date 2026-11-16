#include "server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

#include "http.h"

namespace {

using Json = nlohmann::json;

void SendJson(int fd, int status, const Json& body) {
  SendResponse(fd, status, "application/json",
               body.dump(-1, ' ', false, Json::error_handler_t::replace));
}

void SendError(int fd, int status, const std::string& message) {
  SendJson(fd, status, {{"error", message}});
}

}  // namespace

Server::Server(int port) : port_(port) {}

void Server::Run() {
  const int listener = socket(AF_INET, SOCK_STREAM, 0);
  const int enable = 1;
  setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  address.sin_port = htons(static_cast<uint16_t>(port_));
  if (bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) !=
          0 ||
      listen(listener, SOMAXCONN) != 0) {
    std::perror("listen");
    std::exit(1);
  }
  std::cout << "listening on port " << port_ << std::endl;

  while (true) {
    const int client = accept(listener, nullptr, nullptr);
    if (client < 0) continue;
    HandleClient(client);
    close(client);
  }
}

void Server::HandleClient(int fd) {
  const std::optional<HttpRequest> request = ReadRequest(fd);
  if (!request) return;

  if (request->method == "GET" && request->path == "/api/health") {
    HandleHealth(fd);
  } else {
    SendError(fd, 404, "not found");
  }
}

void Server::HandleHealth(int fd) {
  SendJson(fd, 200, {{"status", "ok"}, {"ollama", false}});
}
