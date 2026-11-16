#ifndef SRC_HTTP_H_
#define SRC_HTTP_H_

#include <cstddef>
#include <map>
#include <optional>
#include <string>

struct HttpRequest {
  std::string method;
  std::string path;
  std::map<std::string, std::string> headers;
  std::string body;
};

bool SendAll(int fd, const void* data, size_t size);

bool RecvAll(int fd, void* data, size_t size);

std::optional<HttpRequest> ReadRequest(int fd);

void SendResponse(int fd, int status, const std::string& content_type,
                  const std::string& body);

#endif  // SRC_HTTP_H_
