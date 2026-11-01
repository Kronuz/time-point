cmake_minimum_required(VERSION 3.14)
project(time_point LANGUAGES CXX)

# --- Library target ---
# time_point is header-only: one header (time_point.hh) with helpers that convert a
# std::chrono::time_point to and from a nanoseconds-since-epoch integer, signed or
# unsigned. It re-exports nanosleep() from the nanosleep library rather than defining its
# own, so both headers can share a translation unit.
include(FetchContent)
FetchContent_Declare(
	nanosleep
	GIT_REPOSITORY https://github.com/Kronuz/nanosleep.git
	GIT_TAG        256aaf62e70c37239798779fa6b305a9c0491803
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
  # One executable per claim, so a failure names it: clocks that do not tick in nanoseconds,
  # instants before the epoch (the signed pair), and the old conversions as a reference.
  foreach(name coarse_clocks pre_epoch diff)
    add_executable(time_point_${name} test/${name}.cc)
    target_link_libraries(time_point_${name} PRIVATE time_point)
    add_test(NAME time_point_${name} COMMAND time_point_${name})
  endforeach()
  # The benchmark (bench/bench.cc), run by hand: the library, and the frozen conversions of 43d8a84
  # twice (the second copy gives the A/A spread). `python3 bench/abcmp.py build` pairs them. The
  # CTest tests run each once; they time nothing.
  option(TIME_POINT_BUILD_BENCH "Build the benchmark in bench/" OFF)
  if(TIME_POINT_BUILD_BENCH)
    add_executable(time_point_bench_new bench/bench.cc)
    target_link_libraries(time_point_bench_new PRIVATE time_point)
    add_test(NAME time_point_bench_new COMMAND time_point_bench_new)
    foreach(copy old old2)
      add_executable(time_point_bench_${copy} bench/bench.cc)
      target_compile_features(time_point_bench_${copy} PRIVATE cxx_std_20)
      target_compile_definitions(time_point_bench_${copy} PRIVATE BENCH_OLD)
      target_include_directories(time_point_bench_${copy} PRIVATE test)
    endforeach()
    add_test(NAME time_point_bench_old COMMAND time_point_bench_old)
  endif()
endif()
