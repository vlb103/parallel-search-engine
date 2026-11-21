#ifndef SRC_OLLAMA_CLIENT_H_
#define SRC_OLLAMA_CLIENT_H_

#include <mutex>
#include <optional>
#include <string>
#include <vector>

class OllamaClient {
 public:
  OllamaClient(std::string host, int port, std::string model);

  bool IsAvailable() const;
  std::vector<std::string> Expand(const std::string& query);

 private:
  std::optional<std::string> Request(const std::string& method,
                                     const std::string& path,
                                     const std::string& body) const;

  const std::string host_;
  const int port_;
  const std::string model_;
  std::mutex generate_mutex_;
};

#endif  // SRC_OLLAMA_CLIENT_H_
