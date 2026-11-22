#ifndef SRC_SERVER_H_
#define SRC_SERVER_H_

#include <cstddef>
#include <string>

#include "http.h"
#include "ollama_client.h"
#include "search_engine.h"

class Server {
 public:
  Server(SearchEngine* engine, OllamaClient* ollama, int port,
         size_t thread_count);

  void Run();

 private:
  void HandleClient(int fd);
  void HandleHealth(int fd);
  void HandleIndex(int fd, const HttpRequest& request);
  void HandleSearch(int fd, const HttpRequest& request);
  void HandleStats(int fd);
  void HandleWebSocket(int fd, const HttpRequest& request);

  SearchEngine* engine_;
  OllamaClient* ollama_;
  int port_;
  size_t thread_count_;
  std::string index_page_;
};

#endif  // SRC_SERVER_H_
