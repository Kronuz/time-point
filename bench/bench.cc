// Benchmark rows for the conversions in time_point.hh. The same source is built twice: against
// the library (time_point_bench_new) and against the frozen conversions of 43d8a84 in
// test/old/time_point_old.hh (time_point_bench_old, BENCH_OLD; time_point_bench_old2 is a second
// copy of it, whose ratio to the first is the A/A spread of the run). Rows that need the new
// API are compiled out of the old binaries. It prints "row ns/op" lines; bench/abcmp.py runs
// the binaries back to back and pairs them.
//
// Every row reads its input through a pointer the compiler cannot see through (launder), so it
// cannot hoist the loop out of the repetition and measures the loop, not one pass of it.
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <vector>

#ifdef BENCH_OLD
#include "old/time_point_old.hh"
using namespace time_point_old;
#else
#include "time_point.hh"
#endif

namespace {

using clk = std::chrono::steady_clock;

struct rng_t {
	std::uint64_t s = 0x9E3779B97F4A7C15ULL;
	std::uint64_t next() {
		s ^= s >> 12;
		s ^= s << 25;
		s ^= s >> 27;
		return s * 0x2545F4914F6CDD1DULL;
	}
};

template <typename T>
inline void keep(const T& v) {
	asm volatile("" : : "r,m"(v) : "memory");
}

template <typename T>
inline T* hide(T* p) {
	asm volatile("" : "+r"(p));
	return p;
}

// A macro, not a function named launder: std::launder is found by argument-dependent lookup
// for std types and makes the call ambiguous.
#define launder(p) hide(p)

// Best of 9 runs of f(), in nanoseconds per operation; each run is repeated to take about 15 ms.
template <typename F>
__attribute__((noinline)) double bench(F&& f, std::size_t ops) {
	f();
	auto t0 = clk::now();
	f();
	double once = std::chrono::duration<double>(clk::now() - t0).count();
	int reps = once > 0 ? static_cast<int>(0.015 / once) + 1 : 1000;
	double best = 1e30;
	for (int r = 0; r < 9; ++r) {
		auto s = clk::now();
		for (int i = 0; i < reps; ++i) f();
		double e = std::chrono::duration<double>(clk::now() - s).count();
		best = std::min(best, e * 1e9 / (static_cast<double>(reps) * static_cast<double>(ops)));
	}
	return best;
}

void row(const char* name, double ns) {
	std::printf("%-44s %8.3f\n", name, ns);
}

const std::size_t N = 4096;

}  // namespace

int main() {
	rng_t rng;
	std::vector<clk::time_point> steady(N);
	std::vector<std::chrono::system_clock::time_point> wall(N);
	std::vector<unsigned long long> counts(N);
	for (std::size_t i = 0; i < N; ++i) {
		steady[i] = clk::time_point(std::chrono::nanoseconds(static_cast<long long>(rng.next() >> 2)));
		wall[i] = std::chrono::system_clock::time_point(std::chrono::system_clock::duration(static_cast<long long>(rng.next() >> 12)));
		counts[i] = rng.next() >> 1;
	}
	row("time_point_to_ullong(steady tp)", bench([&] {
		const clk::time_point* p = launder(steady.data());
		unsigned long long s = 0;
		for (std::size_t i = 0; i < N; ++i) s += time_point_to_ullong(p[i]);
		keep(s);
	}, N));
	row("time_point_from_ullong<steady>(n)", bench([&] {
		const unsigned long long* p = launder(counts.data());
		unsigned long long s = 0;
		for (std::size_t i = 0; i < N; ++i) s += static_cast<unsigned long long>(time_point_from_ullong(p[i]).time_since_epoch().count());
		keep(s);
	}, N));
	row("time_point_to_ullong(system tp)", bench([&] {
		const std::chrono::system_clock::time_point* p = launder(wall.data());
		unsigned long long s = 0;
		for (std::size_t i = 0; i < N; ++i) s += time_point_to_ullong(p[i]);
		keep(s);
	}, N));
	row("from_ullong(to_ullong(steady tp)) round trip", bench([&] {
		const clk::time_point* p = launder(steady.data());
		unsigned long long s = 0;
		for (std::size_t i = 0; i < N; ++i) s += static_cast<unsigned long long>(time_point_from_ullong(time_point_to_ullong(p[i])).time_since_epoch().count());
		keep(s);
	}, N));
	row("time_point_to_ullong(system_clock::now())", bench([&] {
		unsigned long long s = 0;
		for (std::size_t i = 0; i < N / 16; ++i) s += time_point_to_ullong(std::chrono::system_clock::now());
		keep(s);
	}, N / 16));
#ifndef BENCH_OLD
	row("time_point_from_ullong<system_clock>(n)", bench([&] {
		const unsigned long long* p = launder(counts.data());
		unsigned long long s = 0;
		for (std::size_t i = 0; i < N; ++i) s += static_cast<unsigned long long>(time_point_from_ullong<std::chrono::system_clock>(p[i]).time_since_epoch().count());
		keep(s);
	}, N));
	row("time_point_to_llong(steady tp)", bench([&] {
		const clk::time_point* p = launder(steady.data());
		unsigned long long s = 0;
		for (std::size_t i = 0; i < N; ++i) s += static_cast<unsigned long long>(time_point_to_llong(p[i]));
		keep(s);
	}, N));
	row("time_point_from_llong<steady>(n)", bench([&] {
		const unsigned long long* p = launder(counts.data());
		unsigned long long s = 0;
		for (std::size_t i = 0; i < N; ++i) s += static_cast<unsigned long long>(time_point_from_llong(static_cast<long long>(p[i])).time_since_epoch().count());
		keep(s);
	}, N));
#endif
	return 0;
}
