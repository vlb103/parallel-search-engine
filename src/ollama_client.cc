#include "ollama_client.h"

#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <nlohmann/json.hpp>
#include <utility>

#include "http.h"
#include "tokenizer.h"

namespace {

using Json = nlohmann::json;

const int kTimeoutSeconds = 120;
const size_t kReadChunkSize = 4096;
const size_t kMaxExpandedTerms = 8;
const int kMaxGeneratedTokens = 64;
const char kHeaderEnd[] = "\r\n\r\n";
const char kPrompt[] =
    "You expand search queries for a keyword search engine over English "
    "documents. Give 5 English keywords or synonyms related to the query. "
    "Respond with JSON of the form {\"keywords\": [\"word1\", \"word2\"]}.\n"
    "Query: ";

int Connect(const std::string& host, int port) {
  addrinfo hints{};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  addrinfo* address = nullptr;
  if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints,
                  &address) != 0) {
    return -1;
  }

  int fd =
      socket(address->ai_family, address->ai_socktype, address->ai_protocol);
  if (fd >= 0) {
    const timeval timeout{kTimeoutSeconds, 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    if (connect(fd, address->ai_addr, address->ai_addrlen) != 0) {
      close(fd);
      fd = -1;
    }
  }
  freeaddrinfo(address);
  return fd;
}

}  // namespace

OllamaClient::OllamaClient(std::string host, int port, std::string model)
    : host_(std::move(host)), port_(port), model_(std::move(model)) {}

bool OllamaClient::IsAvailable() const {
  return Request("GET", "/api/tags", "").has_value();
}

std::vector<std::string> OllamaClient::Expand(const std::string& query) {
  const Json body = {
      {"model", model_},
      {"prompt", kPrompt + query},
      {"stream", false},
      {"format", "json"},
      {"options", {{"temperature", 0}, {"num_predict", kMaxGeneratedTokens}}}};
  std::optional<std::string> response;
  {
    std::lock_guard<std::mutex> lock(generate_mutex_);
    response =
        Request("POST", "/api/generate",
                body.dump(-1, ' ', false, Json::error_handler_t::replace));
  }
  if (!response) return {};

  std::vector<std::string> terms;
  try {
    const std::string generated =
        Json::parse(*response).at("response").get<std::string>();
    const Json keywords = Json::parse(generated).at("keywords");
    for (const Json& keyword : keywords) {
      for (const std::string& term : Tokenize(keyword.get<std::string>())) {
        if (terms.size() < kMaxExpandedTerms &&
            std::find(terms.begin(), terms.end(), term) == terms.end()) {
          terms.push_back(term);
        }
      }
    }
  } catch (const Json::exception&) {
    return {};
  }
  return terms;
}

std::optional<std::string> OllamaClient::Request(
    const std::string& method, const std::string& path,
    const std::string& body) const {
  const int fd = Connect(host_, port_);
  if (fd < 0) return std::nullopt;

  const std::string request = method + " " + path +
                              " HTTP/1.0\r\nHost: " + host_ +
                              "\r\nContent-Type: application/json"
                              "\r\nContent-Length: " +
                              std::to_string(body.size()) + "\r\n\r\n" + body;
  if (!SendAll(fd, request.data(), request.size())) {
    close(fd);
    return std::nullopt;
  }

  std::string response;
  char chunk[kReadChunkSize];
  ssize_t n;
  while ((n = recv(fd, chunk, sizeof(chunk), 0)) > 0) {
    response.append(chunk, static_cast<size_t>(n));
  }
  close(fd);

  const size_t line_end = response.find("\r\n");
  const size_t header_end = response.find(kHeaderEnd);
  if (response.rfind("HTTP/1.", 0) != 0 || line_end == std::string::npos ||
      response.substr(0, line_end).find(" 200 ") == std::string::npos ||
      header_end == std::string::npos) {
    return std::nullopt;
  }
  return response.substr(header_end + sizeof(kHeaderEnd) - 1);
}
