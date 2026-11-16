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
  src/thread_pool.cc)
target_link_libraries(search_server PRIVATE
  Threads::Threads nlohmann_json::nlohmann_json)
