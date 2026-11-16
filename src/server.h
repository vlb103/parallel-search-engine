#ifndef SRC_SERVER_H_
#define SRC_SERVER_H_

class Server {
 public:
  explicit Server(int port);

  void Run();

 private:
  void HandleClient(int fd);
  void HandleHealth(int fd);

  int port_;
};

#endif  // SRC_SERVER_H_
