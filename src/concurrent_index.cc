#include "concurrent_index.h"

#include <mutex>
#include <utility>

ConcurrentIndex::ConcurrentIndex(size_t segment_count, size_t initial_buckets)
    : segments_(segment_count), initial_buckets_(initial_buckets) {
  Clear();
}

size_t ConcurrentIndex::Hash(const std::string& term) {
  size_t hash = 0;
  for (unsigned char c : term) hash = hash * 31 + c;
  return hash;
}

size_t ConcurrentIndex::BucketOf(const Segment& segment, size_t hash) const {
  return (hash / segments_.size()) % segment.buckets.size();
}

void ConcurrentIndex::Add(const std::string& term, Posting posting) {
  const size_t hash = Hash(term);
  Segment& segment = segments_[hash % segments_.size()];
  std::unique_lock<std::shared_mutex> lock(segment.mutex);

  for (Node* node = segment.buckets[BucketOf(segment, hash)].get();
       node != nullptr; node = node->next.get()) {
    if (node->hash == hash && node->term == term) {
      node->postings.push_back(posting);
      return;
    }
  }

  if (segment.term_count >= segment.buckets.size()) Rehash(&segment);

  std::unique_ptr<Node>& head = segment.buckets[BucketOf(segment, hash)];
  auto node = std::make_unique<Node>();
  node->term = term;
  node->hash = hash;
  node->postings.push_back(posting);
  node->next = std::move(head);
  head = std::move(node);
  ++segment.term_count;
}

void ConcurrentIndex::Rehash(Segment* segment) {
  std::vector<std::unique_ptr<Node>> bigger(segment->buckets.size() * 2);
  for (std::unique_ptr<Node>& head : segment->buckets) {
    while (head != nullptr) {
      std::unique_ptr<Node> node = std::move(head);
      head = std::move(node->next);
      std::unique_ptr<Node>& slot =
          bigger[(node->hash / segments_.size()) % bigger.size()];
      node->next = std::move(slot);
      slot = std::move(node);
    }
  }
  segment->buckets = std::move(bigger);
}

std::vector<Posting> ConcurrentIndex::Find(const std::string& term) const {
  const size_t hash = Hash(term);
  const Segment& segment = segments_[hash % segments_.size()];
  std::shared_lock<std::shared_mutex> lock(segment.mutex);

  for (const Node* node = segment.buckets[BucketOf(segment, hash)].get();
       node != nullptr; node = node->next.get()) {
    if (node->hash == hash && node->term == term) return node->postings;
  }
  return {};
}

void ConcurrentIndex::Clear() {
  for (Segment& segment : segments_) {
    std::unique_lock<std::shared_mutex> lock(segment.mutex);
    segment.buckets.clear();
    segment.buckets.resize(initial_buckets_);
    segment.term_count = 0;
  }
}

size_t ConcurrentIndex::Size() const {
  size_t total = 0;
  for (const Segment& segment : segments_) {
    std::shared_lock<std::shared_mutex> lock(segment.mutex);
    total += segment.term_count;
  }
  return total;
}
