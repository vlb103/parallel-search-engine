#ifndef SRC_OLLAMA_CLIENT_H_
#define SRC_OLLAMA_CLIENT_H_

#include <optional>
#include <string>

class OllamaClient {
 public:
  OllamaClient(std::string host, int port, std::string model);

  bool IsAvailable() const;

 private:
  std::optional<std::string> Request(const std::string& method,
                                     const std::string& path,
                                     const std::string& body) const;

  const std::string host_;
  const int port_;
  const std::string model_;
};

#endif  // SRC_OLLAMA_CLIENT_H_
