#include "search_engine.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
#include <unordered_map>
#include <utility>

#include "thread_pool.h"
#include "tokenizer.h"

namespace {

const size_t kMaxResults = 10;

}  // namespace

SearchEngine::SearchEngine(std::string data_dir)
    : data_dir_(std::move(data_dir)) {}

std::optional<IndexStats> SearchEngine::BuildIndex(size_t threads) {
  std::unique_lock<std::mutex> build_lock(build_mutex_, std::try_to_lock);
  if (!build_lock.owns_lock()) return std::nullopt;

  {
    std::unique_lock<std::shared_mutex> lock(documents_mutex_);
    index_.Clear();
    documents_.clear();
    documents_indexed_ = 0;
  }

  const auto start = std::chrono::steady_clock::now();

  std::vector<std::string> files;
  if (std::filesystem::is_directory(data_dir_)) {
    for (const auto& entry :
         std::filesystem::recursive_directory_iterator(data_dir_)) {
      if (entry.is_regular_file()) {
        files.push_back(std::filesystem::relative(entry.path(), data_dir_)
                            .generic_string());
      }
    }
  }
  std::sort(files.begin(), files.end());
  {
    std::unique_lock<std::shared_mutex> lock(documents_mutex_);
    documents_ = std::move(files);
  }

  ThreadPool pool(threads);
  for (uint32_t doc_id = 0; doc_id < documents_.size(); ++doc_id) {
    pool.Submit([this, doc_id] { IndexDocument(doc_id); });
  }
  pool.Shutdown();

  const auto elapsed = std::chrono::steady_clock::now() - start;
  build_time_ms_ =
      std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
  threads_used_ = threads;
  return Stats();
}

void SearchEngine::IndexDocument(uint32_t doc_id) {
  std::ifstream file(data_dir_ + "/" + documents_[doc_id], std::ios::binary);
  const std::string text((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

  std::unordered_map<std::string, uint32_t> frequencies;
  for (const std::string& term : Tokenize(text)) ++frequencies[term];

  for (const auto& [term, tf] : frequencies) index_.Add(term, {doc_id, tf});
  ++documents_indexed_;
}

IndexStats SearchEngine::Stats() const {
  return {documents_indexed_, build_time_ms_, threads_used_};
}

std::vector<SearchResult> SearchEngine::Search(
    const std::vector<std::string>& terms) const {
  std::shared_lock<std::shared_mutex> lock(documents_mutex_);
  const double total = static_cast<double>(documents_.size());

  std::unordered_map<uint32_t, double> scores;
  const std::set<std::string> unique_terms(terms.begin(), terms.end());
  for (const std::string& term : unique_terms) {
    const std::vector<Posting> postings = index_.Find(term);
    if (postings.empty()) continue;
    const double idf = std::log(1.0 + total / postings.size());
    for (const Posting& posting : postings) {
      scores[posting.doc_id] += (1.0 + std::log(posting.tf)) * idf;
    }
  }

  std::vector<std::pair<uint32_t, double>> ranked(scores.begin(), scores.end());
  const size_t count = std::min(kMaxResults, ranked.size());
  std::partial_sort(ranked.begin(), ranked.begin() + count, ranked.end(),
                    [](const auto& a, const auto& b) {
                      return a.second != b.second ? a.second > b.second
                                                  : a.first < b.first;
                    });

  std::vector<SearchResult> results;
  for (size_t i = 0; i < count; ++i) {
    results.push_back({documents_[ranked[i].first], ranked[i].second, ""});
  }
  return results;
}
