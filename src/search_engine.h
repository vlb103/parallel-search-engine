#ifndef SRC_SEARCH_ENGINE_H_
#define SRC_SEARCH_ENGINE_H_

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <vector>

#include "concurrent_index.h"

struct SearchResult {
  std::string doc_id;
  double score;
  std::string snippet;
};

struct IndexStats {
  size_t documents;
  int64_t build_time_ms;
  size_t threads_used;
};

class SearchEngine {
 public:
  explicit SearchEngine(std::string data_dir);

  std::optional<IndexStats> BuildIndex(size_t threads);

  std::vector<SearchResult> Search(const std::vector<std::string>& terms) const;

  IndexStats Stats() const;

 private:
  void IndexDocument(uint32_t doc_id);
  std::string MakeSnippet(const std::string& doc_id,
                          const std::vector<std::string>& terms) const;

  const std::string data_dir_;
  ConcurrentIndex index_;

  mutable std::shared_mutex documents_mutex_;
  std::vector<std::string> documents_;

  std::mutex build_mutex_;
  std::atomic<size_t> documents_indexed_{0};
  std::atomic<int64_t> build_time_ms_{0};
  std::atomic<size_t> threads_used_{0};
};

#endif  // SRC_SEARCH_ENGINE_H_
