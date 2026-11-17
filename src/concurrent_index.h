#ifndef SRC_CONCURRENT_INDEX_H_
#define SRC_CONCURRENT_INDEX_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <shared_mutex>
#include <string>
#include <vector>

struct Posting {
  uint32_t doc_id;
  uint32_t tf;
};

class ConcurrentIndex {
 public:
  explicit ConcurrentIndex(size_t segment_count = 64,
                           size_t initial_buckets = 1024);

  void Add(const std::string& term, Posting posting);
  std::vector<Posting> Find(const std::string& term) const;
  void Clear();
  size_t Size() const;

 private:
  struct Node {
    std::string term;
    size_t hash;
    std::vector<Posting> postings;
    std::unique_ptr<Node> next;
  };

  struct Segment {
    mutable std::shared_mutex mutex;
    std::vector<std::unique_ptr<Node>> buckets;
    size_t term_count = 0;
  };

  static size_t Hash(const std::string& term);
  void Rehash(Segment* segment);
  size_t BucketOf(const Segment& segment, size_t hash) const;

  std::vector<Segment> segments_;
  size_t initial_buckets_;
};

#endif  // SRC_CONCURRENT_INDEX_H_
