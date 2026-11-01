// Clocks with chosen ticks for the tests. The standard clocks differ by library (libc++'s
// system_clock ticks in microseconds, libstdc++'s in nanoseconds); these do not, so a test
// that needs a coarse or a fine clock gets one on every platform.
#pragma once

#include <chrono>
#include <cstdint>
#include <ratio>

template <typename Rep, typename Period>
struct tick_clock {
	using rep = Rep;
	using period = Period;
	using duration = std::chrono::duration<Rep, Period>;
	using time_point = std::chrono::time_point<tick_clock>;
	static constexpr bool is_steady = true;
	static time_point now() noexcept { return time_point(duration(0)); }
};

using ns_clock = tick_clock<std::int64_t, std::nano>;
using us_clock = tick_clock<std::int64_t, std::micro>;
using ms_clock = tick_clock<std::int64_t, std::milli>;
using s_clock = tick_clock<std::int64_t, std::ratio<1>>;
using min_clock = tick_clock<std::int64_t, std::ratio<60>>;
using odd_clock = tick_clock<std::int64_t, std::ratio<3, 2>>;      // 1.5 s per tick
using ps_clock = tick_clock<std::int64_t, std::pico>;              // finer than a nanosecond
using picoseconds = std::chrono::duration<std::int64_t, std::pico>;
using f_clock = tick_clock<double, std::nano>;                     // floating-point ticks

// xorshift64*: a fixed stream, so a failure reproduces.
struct rng_t {
	std::uint64_t s;
	explicit rng_t(std::uint64_t seed) : s(seed ? seed : 0x9E3779B97F4A7C15ULL) {}
	std::uint64_t next() {
		s ^= s >> 12;
		s ^= s << 25;
		s ^= s >> 27;
		return s * 0x2545F4914F6CDD1DULL;
	}
};
