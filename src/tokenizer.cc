#include "tokenizer.h"

#include <cctype>
#include <cstddef>
#include <unordered_set>

namespace {

const std::unordered_set<std::string> kStopWords = {
    "a",    "an",  "and",  "are", "as",   "at", "be",  "but",  "by",   "for",
    "from", "has", "have", "he",  "in",   "is", "it",  "its",  "not",  "of",
    "on",   "or",  "that", "the", "this", "to", "was", "were", "with", "you"};

const size_t kMinTermLength = 2;

}  // namespace

std::vector<std::string> Tokenize(std::string_view text) {
  std::vector<std::string> terms;
  std::string current;
  auto flush = [&] {
    if (current.size() >= kMinTermLength && kStopWords.count(current) == 0) {
      terms.push_back(current);
    }
    current.clear();
  };
  for (unsigned char c : text) {
    if (std::isalnum(c)) {
      current.push_back(static_cast<char>(std::tolower(c)));
    } else {
      flush();
    }
  }
  flush();
  return terms;
}
