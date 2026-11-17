#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

#include "../src/concurrent_index.h"
#include "../src/thread_pool.h"

namespace {

const uint32_t kWriters = 8;
const uint32_t kReaders = 4;
const uint32_t kTerms = 20000;

void Check(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAILED: " << message << '\n';
    std::exit(1);
  }
}

}  // namespace

int main() {
  ConcurrentIndex index(4, 8);
  std::atomic<bool> writing{true};

  {
    ThreadPool pool(kReaders);
    for (uint32_t reader = 0; reader < kReaders; ++reader) {
      pool.Submit([&index, &writing] {
        uint32_t i = 0;
        while (writing) index.Find("term" + std::to_string(i++ % kTerms));
      });
    }
    {
      ThreadPool writers(kWriters);
      for (uint32_t writer = 0; writer < kWriters; ++writer) {
        writers.Submit([&index, writer] {
          for (uint32_t i = 0; i < kTerms; ++i) {
            index.Add("term" + std::to_string(i), {writer, 1});
          }
        });
      }
    }
    writing = false;
  }

  Check(index.Size() == kTerms, "every term is stored exactly once");
  for (uint32_t i = 0; i < kTerms; ++i) {
    Check(index.Find("term" + std::to_string(i)).size() == kWriters,
          "every writer's posting is present");
  }
  index.Clear();
  Check(index.Size() == 0, "clear removes all terms");

  std::cout << "OK\n";
  return 0;
}
