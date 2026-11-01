// Instants before the epoch, and the signed pair.
//
// time_point_to_ullong returns the nanosecond count as an unsigned integer, so an instant
// before the epoch comes out as its two's complement image, a huge value that sorts after
// every later instant. time_point_to_llong / time_point_from_llong carry the same count as
// a signed integer: before the epoch it is negative and orders like the instants do.
//
// Oracle: std::chrono. The count is nanoseconds(tp.time_since_epoch()) compared with
// std::chrono equality, and the order of the counts is the order of the time_points.
//
// Build via CMake: cmake -B build && cmake --build build && ctest --test-dir build
#include <algorithm>
#include <chrono>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "check.h"
#include "clocks.h"
#include "time_point.hh"

using std::chrono::nanoseconds;
using std::chrono::time_point;
using steady = std::chrono::steady_clock;


static void test_edges() {
	auto before = steady::time_point(nanoseconds(-1));
	CHECK(time_point_to_llong(before) == -1LL);
	CHECK(time_point_to_ullong(before) == ULLONG_MAX);
	CHECK(time_point_from_llong(-1LL) == before);              // steady_clock by default
	CHECK(time_point_from_ullong(ULLONG_MAX) == before);

	auto epoch = steady::time_point(nanoseconds(0));
	CHECK(time_point_to_llong(epoch) == 0LL);
	CHECK(time_point_to_ullong(epoch) == 0ULL);
	CHECK(time_point_from_llong(0LL) == epoch);

	auto first = steady::time_point(nanoseconds::min());
	auto last = steady::time_point(nanoseconds::max());
	CHECK(time_point_to_llong(first) == LLONG_MIN);
	CHECK(time_point_to_llong(last) == LLONG_MAX);
	CHECK(time_point_to_ullong(first) == (1ULL << 63));
	CHECK(time_point_to_ullong(last) == static_cast<unsigned long long>(LLONG_MAX));
	CHECK(time_point_from_llong(LLONG_MIN) == first);
	CHECK(time_point_from_llong(LLONG_MAX) == last);
	CHECK(time_point_from_ullong(1ULL << 63) == first);
	CHECK(time_point_from_ullong(static_cast<unsigned long long>(LLONG_MAX)) == last);
	std::printf("time_point edges OK: -1 ns, the epoch and the int64 limits\n");
}


// For every pair of instants in a window around the epoch, the signed counts order like the
// time_points; the unsigned ones do too except across the epoch, where before-epoch instants
// are huge.
template <typename C>
static void check_order_window(long long lo, long long hi) {
	for (long long a = lo; a <= hi; ++a) {
		time_point<C> ta{typename C::duration(a)};
		long long la = time_point_to_llong(ta);
		unsigned long long ua = time_point_to_ullong(ta);
		for (long long b = lo; b <= hi; ++b) {
			time_point<C> tb{typename C::duration(b)};
			long long lb = time_point_to_llong(tb);
			unsigned long long ub = time_point_to_ullong(tb);
			CHECK((ta < tb) == (la < lb));
			CHECK((ta == tb) == (la == lb));
			if ((a < 0) == (b < 0)) {
				CHECK((ta < tb) == (ua < ub));       // same side of the epoch: unsigned order holds
			} else if (a < 0) {
				CHECK(ua > ub);                      // before the epoch sorts last as unsigned
			}
		}
	}
}


static void test_order() {
	check_order_window<ns_clock>(-1000, 1000);
	check_order_window<us_clock>(-1000, 1000);

	rng_t rng(0xFEE1DEADULL);
	std::vector<steady::time_point> v;
	for (int i = 0; i < 100000; ++i) {
		v.emplace_back(nanoseconds(static_cast<long long>(rng.next())));
	}
	std::sort(v.begin(), v.end());
	for (std::size_t i = 1; i < v.size(); ++i) {
		CHECK(time_point_to_llong(v[i - 1]) <= time_point_to_llong(v[i]));
	}
	std::printf("time_point order OK: signed counts sort like the instants\n");
}


template <typename C>
static void check_round_trip(typename C::duration d) {
	time_point<C> tp{d};
	long long n = time_point_to_llong(tp);
	CHECK(nanoseconds(n) == d);
	CHECK(time_point_from_llong<C>(n) == tp);
	CHECK(time_point_to_ullong(tp) == static_cast<unsigned long long>(n));
	CHECK(time_point_from_ullong<C>(time_point_to_ullong(tp)) == tp);
}


// The signed pair on clocks coarser than a nanosecond: n nanoseconds maps to the last tick not
// after it (0 <= n - tick start < one tick, in std::chrono comparisons), before the epoch too.
template <typename C>
static void check_signed_floor(long long limit) {
	const long long tick = std::chrono::duration_cast<nanoseconds>(typename C::duration(1)).count();
	rng_t rng(0x51611ULL ^ static_cast<std::uint64_t>(tick));
	auto check = [](long long n) {
		auto r = time_point_from_llong<C>(n);
		auto gap = nanoseconds(n) - r.time_since_epoch();
		CHECK(gap >= nanoseconds(0));
		CHECK(gap < typename C::duration(1));
		CHECK(nanoseconds(time_point_to_llong(r)) == r.time_since_epoch());
		CHECK(time_point_from_llong<C>(time_point_to_llong(r)) == r);
	};
	for (long long k = -3; k <= 3; ++k) {
		for (long long d = -2500; d <= 2500; ++d) {
			long long n = k * tick + d;
			if (n >= -limit && n <= limit) check(n);
		}
	}
	for (int i = 0; i < 20000; ++i) {
		check(static_cast<long long>(rng.next() % (2 * static_cast<std::uint64_t>(limit) + 1)) - limit);
	}
}


static void test_signed_coarse() {
	check_signed_floor<us_clock>(4000000000000000000LL);
	check_signed_floor<ms_clock>(4000000000000000000LL);
	check_signed_floor<s_clock>(4000000000000000000LL);
	check_signed_floor<odd_clock>(4000000000000000000LL);
	// The int64 limits, floored: hand-worked quotients, negative ones away from zero.
	auto count = [](auto tp) { return static_cast<long long>(tp.time_since_epoch().count()); };
	CHECK(count(time_point_from_llong<us_clock>(LLONG_MAX)) == 9223372036854775LL);
	CHECK(count(time_point_from_llong<us_clock>(LLONG_MIN)) == -9223372036854776LL);
	CHECK(count(time_point_from_llong<s_clock>(LLONG_MAX)) == 9223372036LL);
	CHECK(count(time_point_from_llong<s_clock>(LLONG_MIN)) == -9223372037LL);
	CHECK(count(time_point_from_llong<s_clock>(-1LL)) == -1LL);
	CHECK(count(time_point_from_llong<s_clock>(-1000000000LL)) == -1LL);
	CHECK(count(time_point_from_llong<s_clock>(-1000000001LL)) == -2LL);
	// A sub-nanosecond tick before the epoch rounds down: -1500 ps is in the nanosecond [-2, -1).
	CHECK(time_point_to_llong(time_point<ps_clock>(picoseconds(-1500))) == -2LL);
	CHECK(time_point_to_llong(time_point<ps_clock>(picoseconds(-1000))) == -1LL);
	CHECK(time_point_to_llong(time_point<ps_clock>(picoseconds(-1))) == -1LL);
	CHECK(time_point_to_llong(time_point<ps_clock>(picoseconds(999))) == 0LL);
	std::printf("time_point signed coarse OK: coarse and fine clocks floor, before the epoch too\n");
}


static void test_round_trip() {
	rng_t rng(0xB0A71ULL);
	for (int i = 0; i < 200000; ++i) {
		check_round_trip<ns_clock>(nanoseconds(static_cast<long long>(rng.next())));
		check_round_trip<steady>(nanoseconds(static_cast<long long>(rng.next())));
		// One in 1000 us ticks, kept within what int64 nanoseconds hold.
		check_round_trip<us_clock>(std::chrono::microseconds(static_cast<long long>(rng.next()) % 9000000000000000LL));
	}

	// The system_clock, a 1969 instant: Apollo 11 landed at 1969-07-20T20:17:40Z, 14182940 s
	// before the epoch, and a half second later.
	using sys = std::chrono::system_clock;
	auto landing = time_point<sys, std::chrono::seconds>(std::chrono::seconds(-14182940));
	auto landing_half = time_point<sys, std::chrono::milliseconds>(std::chrono::milliseconds(-14182940500LL));
	CHECK(time_point_to_llong(landing) == -14182940000000000LL);
	CHECK(time_point_to_llong(landing_half) == -14182940500000000LL);
	auto back = time_point_from_llong<sys>(time_point_to_llong(landing_half));
	CHECK(back == landing_half);
	CHECK(back < landing);
	CHECK(time_point_from_ullong<sys>(time_point_to_ullong(landing_half)) == landing_half);
	std::printf("time_point round trip OK: every instant, before the epoch too\n");
}


int main() {
	test_edges();
	test_order();
	test_round_trip();
	test_signed_coarse();
	std::printf("all time_point pre-epoch tests passed\n");
	return 0;
}
