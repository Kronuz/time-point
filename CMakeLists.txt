cmake_minimum_required(VERSION 3.14)
project(time_point LANGUAGES CXX)

# --- Library target ---
# time_point is header-only and dependency-free: one header (time_point.hh) with
# helpers that convert a std::chrono::time_point to and from a nanoseconds-since-
# epoch unsigned integer, plus a small nanosleep wrapper. Nothing but the standard
# <chrono> and the C <time.h>/<errno.h> behind it. So the target is a pure
# INTERFACE library that just puts the source dir on the consumer's include path.
add_library(time_point INTERFACE)
target_include_directories(time_point INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(time_point INTERFACE cxx_std_20)
# Alias so consumers can link `time_point::time_point` whether they add this via
# add_subdirectory or FetchContent.
add_library(time_point::time_point ALIAS time_point)

# Only build the test when this repo is the top-level project, so consumers
# (e.g. via FetchContent) don't build our tests.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  enable_testing()
  add_executable(time_point_test test/test.cc)
  target_link_libraries(time_point_test PRIVATE time_point)
  add_test(NAME time_point COMMAND time_point_test)
endif()
