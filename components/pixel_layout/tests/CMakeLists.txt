cmake_minimum_required(VERSION 3.16)
project(pixel_layout_tests LANGUAGES CXX)

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

add_library(pixel_layout_lib STATIC
  ../pixel_layout.cpp
  ../widgets.cpp
)
target_include_directories(pixel_layout_lib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/stubs
  ${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_compile_definitions(pixel_layout_lib PUBLIC
  USE_FONT
  USE_IMAGE
  USE_ANIMATION
  USE_TIME
  USE_SENSOR
  USE_TEXT_SENSOR
)

enable_testing()

add_executable(pixel_layout_test
  draw_context_test.cpp
  layout_widgets_test.cpp
  pixel_layout_test.cpp
  perf_test.cpp
)
target_link_libraries(pixel_layout_test
  PRIVATE
    pixel_layout_lib
    GTest::gtest_main
)
target_include_directories(pixel_layout_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
)

include(GoogleTest)
gtest_discover_tests(pixel_layout_test)
