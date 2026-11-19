#include "server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

#include "http.h"
#include "thread_pool.h"
#include "tokenizer.h"

namespace {

using Json = nlohmann::json;

const int kReceiveTimeoutSeconds = 30;
const int64_t kMaxIndexThreads = 256;

void SendJson(int fd, int status, const Json& body) {
  SendResponse(fd, status, "application/json",
               body.dump(-1, ' ', false, Json::error_handler_t::replace));
}

void SendError(int fd, int status, const std::string& message) {
  SendJson(fd, status, {{"error", message}});
}

Json ToJson(const std::vector<SearchResult>& results) {
  Json items = Json::array();
  for (const SearchResult& result : results) {
    items.push_back({{"doc_id", result.doc_id},
                     {"score", result.score},
                     {"snippet", result.snippet}});
  }
  return items;
}

int64_t ElapsedMs(std::chrono::steady_clock::time_point start) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::steady_clock::now() - start)
      .count();
}

}  // namespace

Server::Server(SearchEngine* engine, OllamaClient* ollama, int port,
               size_t thread_count)
    : engine_(engine),
      ollama_(ollama),
      port_(port),
      thread_count_(thread_count) {}

void Server::Run() {
  const int listener = socket(AF_INET, SOCK_STREAM, 0);
  const int enable = 1;
  setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  address.sin_port = htons(static_cast<uint16_t>(port_));
  if (bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) !=
          0 ||
      listen(listener, SOMAXCONN) != 0) {
    std::perror("listen");
    std::exit(1);
  }
  std::cout << "listening on port " << port_ << std::endl;

  ThreadPool pool(thread_count_);
  while (true) {
    const int client = accept(listener, nullptr, nullptr);
    if (client < 0) continue;
    pool.Submit([this, client] {
      HandleClient(client);
      close(client);
    });
  }
}

void Server::HandleClient(int fd) {
  const timeval timeout{kReceiveTimeoutSeconds, 0};
  setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  const std::optional<HttpRequest> request = ReadRequest(fd);
  if (!request) return;

  const std::string& method = request->method;
  const std::string& path = request->path;
  try {
    if (method == "GET" && path == "/api/health") {
      HandleHealth(fd);
    } else if (method == "POST" && path == "/api/index") {
      HandleIndex(fd, *request);
    } else if (method == "POST" && path == "/api/search") {
      HandleSearch(fd, *request);
    } else if (method == "GET" && path == "/api/stats") {
      HandleStats(fd);
    } else {
      SendError(fd, 404, "not found");
    }
  } catch (const Json::exception& error) {
    SendError(fd, 400, error.what());
  }
}

void Server::HandleHealth(int fd) {
  SendJson(fd, 200, {{"status", "ok"}, {"ollama", ollama_->IsAvailable()}});
}

void Server::HandleIndex(int fd, const HttpRequest& request) {
  const Json body = Json::parse(request.body);
  const Json& threads = body.at("threads");
  if (!threads.is_number_integer() || threads.get<int64_t>() < 1 ||
      threads.get<int64_t>() > kMaxIndexThreads) {
    SendError(fd, 400, "threads must be an integer between 1 and 256");
    return;
  }
  const std::optional<IndexStats> stats =
      engine_->BuildIndex(static_cast<size_t>(threads.get<int64_t>()));
  if (!stats) {
    SendError(fd, 409, "indexing already in progress");
    return;
  }
  SendJson(fd, 200,
           {{"status", "ok"},
            {"documents_indexed", stats->documents},
            {"time_ms", stats->build_time_ms}});
}

void Server::HandleStats(int fd) {
  const IndexStats stats = engine_->Stats();
  SendJson(fd, 200,
           {{"documents", stats.documents},
            {"index_build_time_ms", stats.build_time_ms},
            {"threads_used", stats.threads_used}});
}

void Server::HandleSearch(int fd, const HttpRequest& request) {
  const Json body = Json::parse(request.body);
  const Json& query = body.at("query");
  const std::string text = query.is_object()
                               ? query.at("text").get<std::string>()
                               : query.get<std::string>();
  const std::string mode = query.is_object() ? query.value("mode", "keyword")
                                             : body.value("mode", "keyword");
  if (mode != "keyword") {
    SendError(fd, 400, "unsupported mode");
    return;
  }

  const auto start = std::chrono::steady_clock::now();
  const std::vector<SearchResult> results = engine_->Search(Tokenize(text));
  SendJson(fd, 200,
           {{"results", ToJson(results)}, {"time_ms", ElapsedMs(start)}});
}
