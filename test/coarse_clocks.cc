// Clocks that do not tick in nanoseconds, in both directions, through the unsigned pair.
//
// time_point_from_ullong<system_clock> did not compile on libc++, whose system_clock ticks in
// microseconds, and no clock coarser than a nanosecond was accepted. This test builds with
// every one: the real system_clock, and clocks with a chosen tick (clocks.h) so that the
// coarse case is covered on libstdc++ too, where system_clock ticks in nanoseconds. It uses
// only time_point_to_ullong and time_point_from_ullong, so it builds against the old header
// and fails there.
//
// Oracle: std::chrono comparisons only. The instant n nanoseconds after the epoch maps to the
// last tick not after it, that is, 0 <= nanoseconds(n) - r.time_since_epoch() < one tick; the
// test never recomputes the division. The extremes are decimal literals worked out by hand.
//
// Build via CMake: cmake -B build && cmake --build build && ctest --test-dir build
#include <chrono>
#include <climits>
#include <cstdint>
#include <cstdio>

#include "check.h"
#include "clocks.h"
#include "time_point.hh"

using std::chrono::nanoseconds;
using std::chrono::time_point;


// The count as a signed number: a value of 2^63 and up is an instant before the epoch.
template <typename Tp>
static long long to_ll(Tp tp) {
	return static_cast<long long>(time_point_to_ullong(tp));
}

template <typename C>
static time_point<C> from_ll(long long n) {
	return time_point_from_ullong<C>(static_cast<unsigned long long>(n));
}


// The instant n nanoseconds after the epoch.
template <typename C>
static void check_floor(long long n) {
	auto r = from_ll<C>(n);
	auto gap = nanoseconds(n) - r.time_since_epoch();   // common type: nanoseconds
	CHECK(gap >= nanoseconds(0));
	CHECK(gap < typename C::duration(1));
	// And back: the start of that tick.
	CHECK(nanoseconds(to_ll(r)) == r.time_since_epoch());
	CHECK(nanoseconds(to_ll(r)) == nanoseconds(n) - gap);
}


// A tick count k on clock C, through to and back.
template <typename C>
static void check_tick(long long k) {
	time_point<C> tp{typename C::duration(k)};
	CHECK(nanoseconds(to_ll(tp)) == tp.time_since_epoch());
	CHECK(from_ll<C>(to_ll(tp)) == tp);
	CHECK(time_point_from_ullong<C>(time_point_to_ullong(tp)) == tp);
}


// A clock whose tick is a whole number of nanoseconds, at least one.
template <typename C>
static void exercise(long long limit) {
	const long long tick = std::chrono::duration_cast<nanoseconds>(typename C::duration(1)).count();

	// Every nanosecond within 2500 of the first boundaries, on both sides of the epoch.
	for (long long k = -3; k <= 3; ++k) {
		for (long long d = -2500; d <= 2500; ++d) {
			long long n = k * tick + d;
			if (n >= -limit && n <= limit) check_floor<C>(n);
		}
	}

	rng_t rng(0xC0A25EULL ^ static_cast<std::uint64_t>(tick));
	for (int i = 0; i < 20000; ++i) {
		long long n = static_cast<long long>(rng.next() % (2 * static_cast<std::uint64_t>(limit) + 1)) - limit;
		check_floor<C>(n);
	}

	const long long max_ticks = limit / tick;
	for (long long k = -3; k <= 3; ++k) check_tick<C>(k);
	check_tick<C>(max_ticks);
	check_tick<C>(-max_ticks);
	for (int i = 0; i < 20000; ++i) {
		long long k = static_cast<long long>(rng.next() % (2 * static_cast<std::uint64_t>(max_ticks) + 1)) - max_ticks;
		check_tick<C>(k);
	}
}


// The largest and smallest int64 nanosecond counts, floored to the tick, against hand-worked
// quotients (floor, so the negative ones round away from zero).
static void test_extremes() {
	auto count = [](auto tp) { return static_cast<long long>(tp.time_since_epoch().count()); };
	const unsigned long long max = static_cast<unsigned long long>(LLONG_MAX);
	const unsigned long long min = 1ULL << 63;       // LLONG_MIN
	CHECK(count(time_point_from_ullong<us_clock>(max)) == 9223372036854775LL);
	CHECK(count(time_point_from_ullong<us_clock>(min)) == -9223372036854776LL);
	CHECK(count(time_point_from_ullong<ms_clock>(max)) == 9223372036854LL);
	CHECK(count(time_point_from_ullong<ms_clock>(min)) == -9223372036855LL);
	CHECK(count(time_point_from_ullong<s_clock>(max)) == 9223372036LL);
	CHECK(count(time_point_from_ullong<s_clock>(min)) == -9223372037LL);
	CHECK(count(time_point_from_ullong<min_clock>(max)) == 153722867LL);
	CHECK(count(time_point_from_ullong<min_clock>(min)) == -153722868LL);
	CHECK(count(time_point_from_ullong<odd_clock>(max)) == 6148914691LL);
	CHECK(count(time_point_from_ullong<odd_clock>(min)) == -6148914692LL);
	// -1 ns is one tick before the epoch on every coarse clock.
	CHECK(count(time_point_from_ullong<s_clock>(ULLONG_MAX)) == -1LL);
	CHECK(count(time_point_from_ullong<us_clock>(ULLONG_MAX)) == -1LL);
	// And whole ticks and the nanoseconds between them.
	CHECK(count(time_point_from_ullong<us_clock>(999999ULL)) == 999LL);
	CHECK(count(time_point_from_ullong<us_clock>(1000ULL)) == 1LL);
	CHECK(count(time_point_from_ullong<us_clock>(999ULL)) == 0LL);
	std::printf("time_point extremes OK: int64 limits floor to the tick\n");
}


// The standard clocks: whatever tick the library gives them (libc++'s system_clock is
// microseconds, libstdc++'s is nanoseconds), the conversions build and obey the oracle.
template <typename C>
static void check_standard_clock() {
	const long long tick = std::chrono::duration_cast<nanoseconds>(typename C::duration(1)).count();
	const long long samples[] = {
		0LL, 1LL, 999LL, 1000LL, 1001LL, 1500LL, 999999LL, 1000000LL,
		1000000000LL, 1234567890123456789LL, 1719662400123456789LL, 4000000000000000000LL,
		-1LL, -999LL, -1000LL, -1001LL, -1500LL, -999999LL, -1000000000LL, -1234567890123456789LL,
		-4000000000000000000LL,
	};
	for (long long n : samples) check_floor<C>(n);

	auto now = C::now();
	CHECK(time_point_from_ullong<C>(time_point_to_ullong(now)) == now);

	// 2024-06-29 12:00:00.123456789 UTC, a positive instant: the ullong is the plain count,
	// floored to the clock's tick.
	auto t = time_point_from_ullong<C>(1719662400123456789ULL);
	CHECK(std::chrono::duration_cast<std::chrono::seconds>(t.time_since_epoch()).count() == 1719662400);
	CHECK(time_point_to_ullong(t) == 1719662400123456789ULL - 1719662400123456789ULL % static_cast<unsigned long long>(tick));
}


static void test_standard_clocks() {
	check_standard_clock<std::chrono::system_clock>();
	check_standard_clock<std::chrono::steady_clock>();
	check_standard_clock<std::chrono::high_resolution_clock>();
	std::printf("time_point standard clocks OK: system_clock (%lld ns tick), steady_clock, high_resolution_clock\n",
		static_cast<long long>(std::chrono::duration_cast<nanoseconds>(std::chrono::system_clock::duration(1)).count()));
}


// Any std::chrono instant converts, whatever its duration, not only the clock's own.
static void test_other_durations() {
	using std::chrono::duration;
	using std::chrono::milliseconds;
	using std::chrono::seconds;
	CHECK(time_point_to_ullong(time_point<std::chrono::steady_clock, seconds>(seconds(3))) == 3000000000ULL);
	CHECK(to_ll(time_point<std::chrono::steady_clock, seconds>(seconds(-3))) == -3000000000LL);
	CHECK(time_point_to_ullong(time_point<std::chrono::system_clock, milliseconds>(milliseconds(5))) == 5000000ULL);
	CHECK(to_ll(time_point<std::chrono::steady_clock, duration<double>>(duration<double>(-2.5))) == -2500000000LL);
	CHECK(to_ll(time_point<std::chrono::steady_clock, duration<double, std::micro>>(duration<double, std::micro>(-1.5))) == -1500LL);
	// Sub-nanosecond ticks round down to the nanosecond, before the epoch as after it.
	CHECK(to_ll(time_point<std::chrono::steady_clock, duration<double, std::pico>>(duration<double, std::pico>(-1500))) == -2LL);
	CHECK(to_ll(time_point<std::chrono::steady_clock, duration<double, std::pico>>(duration<double, std::pico>(1500))) == 1LL);
	// The clock named explicitly still works.
	CHECK(time_point_to_ullong<std::chrono::steady_clock>(time_point<std::chrono::steady_clock>(nanoseconds(42))) == 42ULL);
	std::printf("time_point durations OK: any std::chrono::time_point<Clock, Duration> converts\n");
}


// A clock finer than a nanosecond: to floors to the nanosecond, from is exact.
static void test_finer_clock() {
	for (long long k = -5000; k <= 5000; ++k) {
		time_point<ps_clock> tp{picoseconds(k)};
		long long n = to_ll(tp);
		auto gap = tp.time_since_epoch() - nanoseconds(n);      // common type: picoseconds
		CHECK(gap >= picoseconds(0));
		CHECK(gap < nanoseconds(1));
	}
	for (long long n = -2000000; n <= 2000000; n += 7) {
		CHECK(from_ll<ps_clock>(n).time_since_epoch() == nanoseconds(n));
	}
	std::printf("time_point finer clock OK: picoseconds floor to the nanosecond, nanoseconds are exact\n");
}


// A clock with floating-point ticks.
static void test_floating_clock() {
	using std::chrono::duration;
	for (long long k = -3000; k <= 3000; ++k) {
		time_point<f_clock> tp{duration<double, std::nano>(static_cast<double>(k) / 4)};   // quarter nanoseconds
		long long n = to_ll(tp);
		auto gap = tp.time_since_epoch() - nanoseconds(n);
		CHECK(gap >= duration<double, std::nano>(0));
		CHECK(gap < duration<double, std::nano>(1));
	}
	for (long long n = -1000000; n <= 1000000; n += 13) {
		CHECK(from_ll<f_clock>(n).time_since_epoch().count() == static_cast<double>(n));
	}
	std::printf("time_point floating clock OK: floating ticks floor to the nanosecond, nanoseconds are exact\n");
}


int main() {
	exercise<ns_clock>(LLONG_MAX / 2);
	exercise<us_clock>(4000000000000000000LL);
	exercise<ms_clock>(4000000000000000000LL);
	exercise<s_clock>(4000000000000000000LL);
	exercise<min_clock>(4000000000000000000LL);
	exercise<odd_clock>(4000000000000000000LL);
	std::printf("time_point coarse clocks OK: ns, us, ms, s, minute and 1.5 s ticks floor to the tick\n");
	test_extremes();
	test_standard_clocks();
	test_other_durations();
	test_finer_clock();
	test_floating_clock();
	std::printf("all time_point coarse clock tests passed\n");
	return 0;
}
