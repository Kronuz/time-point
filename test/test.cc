// Smoke test for the standalone time_point library.
//
// Exercises time_point.hh: the two conversion helpers that move a
// std::chrono::time_point to and from a nanoseconds-since-epoch unsigned integer
// (time_point_to_ullong / time_point_from_ullong), and the nanosleep wrapper.
// The conversions are a lossless round-trip at nanosecond resolution, so the
// test pins down that a value survives the trip both ways and that now() behaves
// sanely (monotonic, advances across a short sleep).
//
// Build via CMake: cmake -B build && cmake --build build && ctest --test-dir build
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdio>

#include "time_point.hh"


// ---------------------------------------------------------------------------
// ullong -> time_point -> ullong is the identity at nanosecond resolution.
//
// time_point_from_ullong<T> builds a T::time_point straight from a nanoseconds
// duration, so it is well-formed only for clocks whose native time_point ticks
// in nanoseconds (steady_clock, and high_resolution_clock which aliases it on
// libc++). system_clock ticks coarser (microseconds on libc++), so feeding it
// nanoseconds wouldn't compile; that asymmetry is part of the header's contract
// and is documented in AGENTS.md. We round-trip the nanosecond clocks here.
// ---------------------------------------------------------------------------

static void test_roundtrip_from_ullong() {
	const unsigned long long values[] = {
		0ULL,
		1ULL,
		1000000000ULL,                 // exactly one second
		1234567890123456789ULL,        // an arbitrary, nanosecond-precise instant
		1719662400000000000ULL,        // 2024-06-29T12:00:00Z-ish in ns since epoch
	};

	for (auto ns : values) {
		// steady_clock is the default template argument.
		auto tp_steady = time_point_from_ullong(ns);
		assert(time_point_to_ullong(tp_steady) == ns);

		// An explicitly-named nanosecond clock works the same way.
		auto tp_hires = time_point_from_ullong<std::chrono::high_resolution_clock>(ns);
		assert(time_point_to_ullong(tp_hires) == ns);
	}

	std::printf("time_point round-trip OK: ullong -> time_point -> ullong is exact\n");
}


// ---------------------------------------------------------------------------
// time_point -> ullong -> time_point is the identity for an existing instant,
// as long as the clock's tick is at least nanosecond-coarse (it is, for the
// standard clocks, after a duration_cast to nanoseconds).
// ---------------------------------------------------------------------------

static void test_roundtrip_from_time_point() {
	auto now = std::chrono::steady_clock::now();
	auto ns = time_point_to_ullong(now);
	auto back = time_point_from_ullong(ns);
	// Going through nanoseconds-since-epoch and back lands on the same nanosecond.
	assert(time_point_to_ullong(back) == ns);
	assert(back == now);

	std::printf("time_point round-trip OK: time_point -> ullong -> time_point is exact\n");
}


// ---------------------------------------------------------------------------
// now() sanity: a freshly-read steady_clock time is non-zero (we are well past
// the epoch in any realistic run) and never goes backwards between two reads.
// ---------------------------------------------------------------------------

static void test_now_monotonic() {
	auto t0 = time_point_to_ullong(std::chrono::steady_clock::now());
	auto t1 = time_point_to_ullong(std::chrono::steady_clock::now());
	assert(t0 > 0);
	assert(t1 >= t0);   // steady_clock never runs backwards

	// time_point_to_ullong duration_casts explicitly, so it accepts any clock's
	// time_point, including coarser ones like system_clock. We are well past the
	// Unix epoch, so the wall-clock value is large and positive.
	auto wall = time_point_to_ullong(std::chrono::system_clock::now());
	assert(wall > 0);

	std::printf("time_point now() OK: non-zero and monotonic non-decreasing\n");
}


// ---------------------------------------------------------------------------
// nanosleep wrapper: a short sleep must actually pass time, measured against an
// independent steady_clock read. We sleep 5ms and require steady_clock to have
// advanced by at least most of it (allowing slop for coarse timers).
// ---------------------------------------------------------------------------

static void test_nanosleep_advances() {
	const unsigned long long sleep_ns = 5000000ULL;   // 5 milliseconds

	auto before = std::chrono::steady_clock::now();
	nanosleep(sleep_ns);
	auto after = std::chrono::steady_clock::now();

	auto elapsed_ns = time_point_to_ullong(after) - time_point_to_ullong(before);
	// Be lenient on the low side: timer granularity and scheduling can shave a
	// little. Requiring half the request still proves the sleep did something.
	assert(elapsed_ns >= sleep_ns / 2);

	// nanosleep(0) is a documented no-op and must return promptly.
	nanosleep(0);

	std::printf("time_point nanosleep OK: a 5ms sleep advances steady_clock\n");
}


int main() {
	test_roundtrip_from_ullong();
	test_roundtrip_from_time_point();
	test_now_monotonic();
	test_nanosleep_advances();
	std::printf("all time_point tests passed\n");
	return 0;
}
