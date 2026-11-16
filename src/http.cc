#include "http.h"

#include <sys/socket.h>
#include <sys/types.h>

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace {

const size_t kMaxHeaderSize = 16 * 1024;
const size_t kMaxBodySize = 1024 * 1024;
const size_t kReadChunkSize = 4096;
const char kHeaderEnd[] = "\r\n\r\n";

const char* StatusText(int status) {
  switch (status) {
    case 200:
      return "OK";
    case 400:
      return "Bad Request";
    case 404:
      return "Not Found";
    case 409:
      return "Conflict";
    default:
      return "Error";
  }
}

std::string ToLower(std::string text) {
  std::transform(text.begin(), text.end(), text.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return text;
}

std::string Trim(const std::string& text) {
  const size_t begin = text.find_first_not_of(" \t\r");
  if (begin == std::string::npos) return "";
  const size_t end = text.find_last_not_of(" \t\r");
  return text.substr(begin, end - begin + 1);
}

std::optional<std::string> ReadHead(int fd, std::string* rest) {
  std::string buffer;
  char chunk[kReadChunkSize];
  size_t header_end;
  while ((header_end = buffer.find(kHeaderEnd)) == std::string::npos) {
    if (buffer.size() > kMaxHeaderSize) return std::nullopt;
    const ssize_t n = recv(fd, chunk, sizeof(chunk), 0);
    if (n <= 0) return std::nullopt;
    buffer.append(chunk, static_cast<size_t>(n));
  }
  *rest = buffer.substr(header_end + sizeof(kHeaderEnd) - 1);
  return buffer.substr(0, header_end);
}

}  // namespace

bool SendAll(int fd, const void* data, size_t size) {
  const char* bytes = static_cast<const char*>(data);
  size_t sent = 0;
  while (sent < size) {
    const ssize_t n = send(fd, bytes + sent, size - sent, MSG_NOSIGNAL);
    if (n <= 0) return false;
    sent += static_cast<size_t>(n);
  }
  return true;
}

bool RecvAll(int fd, void* data, size_t size) {
  char* bytes = static_cast<char*>(data);
  size_t received = 0;
  while (received < size) {
    const ssize_t n = recv(fd, bytes + received, size - received, 0);
    if (n <= 0) return false;
    received += static_cast<size_t>(n);
  }
  return true;
}

std::optional<HttpRequest> ReadRequest(int fd) {
  std::string body;
  const std::optional<std::string> head = ReadHead(fd, &body);
  if (!head) return std::nullopt;

  std::istringstream lines(*head);
  std::string request_line;
  std::getline(lines, request_line);

  HttpRequest request;
  std::istringstream(request_line) >> request.method >> request.path;
  if (request.method.empty() || request.path.empty()) return std::nullopt;

  std::string line;
  while (std::getline(lines, line)) {
    const size_t colon = line.find(':');
    if (colon == std::string::npos) continue;
    request.headers[ToLower(Trim(line.substr(0, colon)))] =
        Trim(line.substr(colon + 1));
  }

  const auto length = request.headers.find("content-length");
  if (length != request.headers.end()) {
    size_t size;
    try {
      size = std::stoul(length->second);
    } catch (const std::exception&) {
      return std::nullopt;
    }
    if (size > kMaxBodySize || body.size() > size) return std::nullopt;
    const size_t missing = size - body.size();
    body.resize(size);
    if (!RecvAll(fd, body.data() + size - missing, missing)) {
      return std::nullopt;
    }
  }
  request.body = std::move(body);
  return request;
}

void SendResponse(int fd, int status, const std::string& content_type,
                  const std::string& body) {
  const std::string head =
      "HTTP/1.1 " + std::to_string(status) + " " + StatusText(status) +
      "\r\nContent-Type: " + content_type +
      "\r\nContent-Length: " + std::to_string(body.size()) +
      "\r\nConnection: close\r\n\r\n";
  SendAll(fd, head.data(), head.size());
  SendAll(fd, body.data(), body.size());
}
