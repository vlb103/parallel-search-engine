#include <cstdlib>
#include <string>

#include "server.h"

namespace {

std::string EnvOr(const char* name, const std::string& fallback) {
  const char* value = std::getenv(name);
  return value != nullptr ? value : fallback;
}

}  // namespace

int main() {
  const int port = std::stoi(EnvOr("PORT", "8080"));

  Server server(port);
  server.Run();
}
