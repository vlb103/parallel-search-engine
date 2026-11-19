#include "ollama_client.h"

#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <utility>

#include "http.h"

namespace {

const int kTimeoutSeconds = 120;
const size_t kReadChunkSize = 4096;
const char kHeaderEnd[] = "\r\n\r\n";

int Connect(const std::string& host, int port) {
  addrinfo hints{};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  addrinfo* address = nullptr;
  if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints,
                  &address) != 0) {
    return -1;
  }

  int fd =
      socket(address->ai_family, address->ai_socktype, address->ai_protocol);
  if (fd >= 0) {
    const timeval timeout{kTimeoutSeconds, 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    if (connect(fd, address->ai_addr, address->ai_addrlen) != 0) {
      close(fd);
      fd = -1;
    }
  }
  freeaddrinfo(address);
  return fd;
}

}  // namespace

OllamaClient::OllamaClient(std::string host, int port, std::string model)
    : host_(std::move(host)), port_(port), model_(std::move(model)) {}

bool OllamaClient::IsAvailable() const {
  return Request("GET", "/api/tags", "").has_value();
}

std::optional<std::string> OllamaClient::Request(
    const std::string& method, const std::string& path,
    const std::string& body) const {
  const int fd = Connect(host_, port_);
  if (fd < 0) return std::nullopt;

  const std::string request = method + " " + path +
                              " HTTP/1.0\r\nHost: " + host_ +
                              "\r\nContent-Type: application/json"
                              "\r\nContent-Length: " +
                              std::to_string(body.size()) + "\r\n\r\n" + body;
  if (!SendAll(fd, request.data(), request.size())) {
    close(fd);
    return std::nullopt;
  }

  std::string response;
  char chunk[kReadChunkSize];
  ssize_t n;
  while ((n = recv(fd, chunk, sizeof(chunk), 0)) > 0) {
    response.append(chunk, static_cast<size_t>(n));
  }
  close(fd);

  const size_t line_end = response.find("\r\n");
  const size_t header_end = response.find(kHeaderEnd);
  if (response.rfind("HTTP/1.", 0) != 0 || line_end == std::string::npos ||
      response.substr(0, line_end).find(" 200 ") == std::string::npos ||
      header_end == std::string::npos) {
    return std::nullopt;
  }
  return response.substr(header_end + sizeof(kHeaderEnd) - 1);
}
