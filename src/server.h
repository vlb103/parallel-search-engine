#ifndef SRC_SERVER_H_
#define SRC_SERVER_H_

#include <cstddef>

class Server {
 public:
  Server(int port, size_t thread_count);

  void Run();

 private:
  void HandleClient(int fd);
  void HandleHealth(int fd);

  int port_;
  size_t thread_count_;
};

#endif  // SRC_SERVER_H_
