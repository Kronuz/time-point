// The library against the conversions it replaced (test/old/time_point_old.hh, frozen).
//
// Everywhere the old code compiled and the instants are whole nanoseconds or coarser, the new
// functions return what it returned, bit for bit. The result differs on exactly one class:
// an instant before the epoch on a clock that ticks finer than a nanosecond and is not a
// whole nanosecond, which the old code truncated toward the epoch and the new code rounds
// down. The test lists it, asserts old != new there, and checks new against std::chrono:
// the nanosecond count n of a tick t satisfies 0 <= t - n ns < 1 ns.
// Clocks coarser than a nanosecond did not compile on the old code, so nothing can be compared.
//
// Build via CMake: cmake -B build && cmake --build build && ctest --test-dir build
#include <chrono>
#include <climits>
#include <cstdint>
#include <cstdio>

#include "check.h"
#include "clocks.h"
#include "old/time_point_old.hh"
#include "time_point.hh"

using std::chrono::nanoseconds;
using std::chrono::time_point;
using steady = std::chrono::steady_clock;


template <typename C>
static void check_nanosecond_clock(rng_t& rng) {
	const unsigned long long edges[] = {
		0ULL, 1ULL, 1000ULL, 1000000000ULL, 1719662400123456789ULL,
		static_cast<unsigned long long>(LLONG_MAX), 1ULL << 63, (1ULL << 63) + 1, ULLONG_MAX - 1, ULLONG_MAX,
	};
	for (unsigned long long n : edges) {
		CHECK(time_point_old::time_point_from_ullong<C>(n) == time_point_from_ullong<C>(n));
		auto tp = time_point_from_ullong<C>(n);
		CHECK(time_point_old::time_point_to_ullong(tp) == time_point_to_ullong(tp));
		CHECK(time_point_to_ullong(tp) == n);
	}
	for (int i = 0; i < 200000; ++i) {
		unsigned long long n = rng.next();
		CHECK(time_point_old::time_point_from_ullong<C>(n) == time_point_from_ullong<C>(n));
		auto tp = time_point_from_ullong<C>(n);
		CHECK(time_point_old::time_point_to_ullong(tp) == time_point_to_ullong(tp));
	}
}


// Sub-nanosecond ticks: a tick that is a whole number of nanoseconds, or after the epoch,
// converts as before; a negative tick that is not whole nanoseconds is the one changed class.
static void check_finer_clock() {
	long long changed = 0;
	for (long long k = -50000; k <= 50000; ++k) {
		time_point<ps_clock> tp{picoseconds(k)};
		unsigned long long old_n = time_point_old::time_point_to_ullong(tp);
		unsigned long long new_n = time_point_to_ullong(tp);
		if (k >= 0 || k % 1000 == 0) {
			CHECK(old_n == new_n);
		} else {
			++changed;
			CHECK(old_n != new_n);
			// New: the nanosecond at or before the tick (std::chrono's difference in [0, 1 ns)).
			auto gap = tp.time_since_epoch() - nanoseconds(static_cast<long long>(new_n));
			CHECK(gap >= picoseconds(0));
			CHECK(gap < nanoseconds(1));
			// Old: the nanosecond toward the epoch, so one later than the new one.
			CHECK(old_n == new_n + 1);
		}
	}
	CHECK(changed == 50000 - 50);   // the negative ticks that are not a multiple of 1000
	// From a nanosecond count onto the fine clock, the result is exact and as before.
	for (long long n = -100000; n <= 100000; ++n) {
		CHECK(time_point_old::time_point_from_ullong<ps_clock>(static_cast<unsigned long long>(n)) ==
			time_point_from_ullong<ps_clock>(static_cast<unsigned long long>(n)));
	}
}


int main() {
	rng_t rng(0xD1FFULL);
	check_nanosecond_clock<steady>(rng);
	check_nanosecond_clock<std::chrono::high_resolution_clock>(rng);
	check_nanosecond_clock<ns_clock>(rng);
	std::printf("time_point differential OK: nanosecond clocks return the old results\n");
	check_finer_clock();
	std::printf("time_point differential OK: sub-nanosecond clocks differ only before the epoch\n");
	std::printf("all time_point differential tests passed\n");
	return 0;
}
