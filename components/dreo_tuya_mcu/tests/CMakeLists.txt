cmake_minimum_required(VERSION 3.16)
project(dreo_tuya_mcu_tests LANGUAGES CXX)

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

add_library(dreo_tuya_protocol STATIC
  ../dreo_tuya_protocol.cpp
)
target_include_directories(dreo_tuya_protocol PUBLIC
  ..
)

enable_testing()

add_executable(dreo_tuya_protocol_test
  dreo_tuya_protocol_test.cpp
)
target_link_libraries(dreo_tuya_protocol_test
  PRIVATE
    dreo_tuya_protocol
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(dreo_tuya_protocol_test)
