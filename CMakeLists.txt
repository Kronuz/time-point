cmake_minimum_required(VERSION 3.14)
project(time_point LANGUAGES CXX)

# --- Library target ---
# time_point is header-only: one header (time_point.hh) with helpers that convert a
# std::chrono::time_point to and from a nanoseconds-since-epoch unsigned integer. It
# re-exports nanosleep() from the nanosleep library rather than defining its own, so
# both headers can share a translation unit.
include(FetchContent)
FetchContent_Declare(
	nanosleep
	GIT_REPOSITORY https://github.com/Kronuz/nanosleep.git
	GIT_TAG        main
)
FetchContent_MakeAvailable(nanosleep)

add_library(time_point INTERFACE)
target_link_libraries(time_point INTERFACE nanosleep::nanosleep)
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
  add_executable(time_point_both_headers test/both_headers.cc)
  target_link_libraries(time_point_both_headers PRIVATE time_point)
  add_test(NAME time_point_both_headers COMMAND time_point_both_headers)
endif()
