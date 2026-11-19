#ifndef SRC_SERVER_H_
#define SRC_SERVER_H_

#include <cstddef>

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

  SearchEngine* engine_;
  OllamaClient* ollama_;
  int port_;
  size_t thread_count_;
};

#endif  // SRC_SERVER_H_
