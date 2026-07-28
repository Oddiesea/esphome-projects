cmake_minimum_required(VERSION 3.16)
project(valence_rt_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_library(valence_protocol STATIC
  ../valence_protocol.cpp
)
target_include_directories(valence_protocol PUBLIC
  ..
)

enable_testing()

add_executable(valence_protocol_test
  valence_protocol_test.cpp
)
target_link_libraries(valence_protocol_test
  PRIVATE
    valence_protocol
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(valence_protocol_test)
