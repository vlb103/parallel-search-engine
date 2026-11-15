#include <cstdlib>
#include <iostream>
#include <string>

namespace {

std::string EnvOr(const char* name, const std::string& fallback) {
  const char* value = std::getenv(name);
  return value != nullptr ? value : fallback;
}

}  // namespace

int main() {
  const int port = std::stoi(EnvOr("PORT", "8080"));
  std::cout << "search_server: port " << port << std::endl;
  return 0;
}
