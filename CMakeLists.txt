cmake_minimum_required(VERSION 3.16)
project(search_server CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(-Wall -Wextra)

find_package(Threads REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(search_server
  src/main.cc
  src/server.cc
  src/http.cc
  src/ollama_client.cc
  src/search_engine.cc
  src/tokenizer.cc
  src/concurrent_index.cc
  src/thread_pool.cc)
target_link_libraries(search_server PRIVATE
  Threads::Threads nlohmann_json::nlohmann_json)

add_executable(index_race_test
  tests/index_race_test.cc
  src/concurrent_index.cc
  src/thread_pool.cc)
target_compile_options(index_race_test PRIVATE -fsanitize=thread -g)
target_link_options(index_race_test PRIVATE -fsanitize=thread)
target_link_libraries(index_race_test PRIVATE Threads::Threads)
