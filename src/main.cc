#include <cstddef>
#include <cstdlib>
#include <string>

#include "search_engine.h"
#include "server.h"

namespace {

std::string EnvOr(const char* name, const std::string& fallback) {
  const char* value = std::getenv(name);
  return value != nullptr ? value : fallback;
}

}  // namespace

int main() {
  const int port = std::stoi(EnvOr("PORT", "8080"));
  const size_t threads = std::stoul(EnvOr("SERVER_THREADS", "16"));

  SearchEngine engine(EnvOr("DATA_DIR", "/data"));
  Server server(&engine, port, threads);
  server.Run();
}
