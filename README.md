# time-point

Part of [Kronuz C++ Libraries](https://github.com/Kronuz/cpp-libraries), a collection of small, reusable C++ libraries.

[![CI](https://github.com/Kronuz/time-point/actions/workflows/ci.yml/badge.svg)](https://github.com/Kronuz/time-point/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![Header-only](https://img.shields.io/badge/header--only-yes-green.svg)

Header-only time_point packing for C++20: a std::chrono instant to one integer of
nanoseconds and back. Both directions pivot on `std::chrono::nanoseconds`, so the value
is the count since the clock's epoch, negative before it. Reach for it to store an
instant in a fixed-width column, a binary record, a queue entry or an atomic.

## ✨ Highlights

- **`time_point_to_llong(tp)`:** nanoseconds since the epoch of `tp`'s clock as a
  `long long`, negative before the epoch; clock and duration are deduced. Counts order
  like the instants.
- **`time_point_to_ullong(tp)`:** the same count as an `unsigned long long`. An instant
  before the epoch is its two's complement image, `2^63` or more, so it sorts after
  every later instant as an unsigned integer.
- **`time_point_from_llong<Clock>(n)` and `time_point_from_ullong<Clock>(n)`:** the
  instant back on `Clock`, which defaults to `std::chrono::steady_clock`. On a clock
  that ticks coarser than a nanosecond (libc++'s `system_clock` ticks in microseconds)
  the instant rounds down to the tick; the unsigned form reads `2^63` and up as before
  the epoch.
- **Exact round trips:** an instant converts to its count and back unchanged on clocks
  that tick in nanoseconds or coarser; a count converts to an instant and back
  unchanged on clocks that tick in nanoseconds or finer.
- **Four function templates** in one header, over `<chrono>`.

## 🚀 Quick start

```cpp
#include "time_point.hh"

#include <chrono>
#include <cstdio>

int main() {
	auto now = std::chrono::steady_clock::now();
	unsigned long long packed = time_point_to_ullong(now);  // nanoseconds since the epoch

	auto back = time_point_from_ullong(packed);  // steady_clock by default
	std::puts(back == now ? "same instant" : "different");  // same instant

	auto t = time_point_from_ullong<std::chrono::steady_clock>(1'500'000'000);
	std::printf("%lld ms\n", static_cast<long long>(
		std::chrono::duration_cast<std::chrono::milliseconds>(t.time_since_epoch()).count()));
	// 1500 ms

	// An instant before the epoch has a negative count, on any clock.
	auto before = std::chrono::system_clock::time_point(std::chrono::seconds(-1));
	std::printf("%lld ns\n", time_point_to_llong(before));  // -1000000000 ns
	std::puts(time_point_from_llong<std::chrono::system_clock>(-1'000'000'000) == before
		? "same instant" : "different");  // same instant
}
```

Convert back on the clock the value came from: each clock has its own epoch.

## 📦 Installation

Requires C++20 and CMake 3.14 or newer. CMake fetches
[nanosleep](https://github.com/Kronuz/nanosleep), header-only, whose `nanosleep(nsec)`
the header re-exports. CI builds and tests it on Linux (x86_64 with GCC and Clang,
arm64), macOS (arm64, x86_64) and FreeBSD, plus ASan, UBSan and TSan builds.

```cmake
include(FetchContent)
FetchContent_Declare(time_point
  GIT_REPOSITORY https://github.com/Kronuz/time-point.git
  GIT_TAG        624c2e9a15fd477b8aa5a405184a30b0caeb9038)  # or main, to track the tip
FetchContent_MakeAvailable(time_point)

target_link_libraries(my_app PRIVATE time_point::time_point)
```

Link `time_point::time_point` and include `time_point.hh`.

## 🎯 When to use it

- Timestamps in binary records, wire formats, queues and atomics, where one integer
  travels better than a `std::chrono` type.

Not for:

- Instants more than about 292 years from the clock's epoch: the count is a 64-bit
  number of nanoseconds, so a wider-ranging `time_point` (for instance seconds in the
  year 3000) overflows it.
- Parsing, formatting or date math: see [datetime](https://github.com/Kronuz/datetime).

## 📚 Documentation

- [ARCHITECTURE.md](ARCHITECTURE.md): the two conversions and why the pivot is
  nanoseconds.
- [AGENTS.md](AGENTS.md): repository map, invariants and traps.
- [Catalog entry](https://github.com/Kronuz/cpp-libraries/blob/main/AGENTS.md#time-point)
  in Kronuz C++ Libraries.

## 🛠️ Building and testing

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

## 🤝 Contributing

Issues and pull requests are welcome; read [AGENTS.md](AGENTS.md) and the catalog's
[rules of engagement](https://github.com/Kronuz/cpp-libraries/blob/main/AGENTS.md#rules-of-engagement)
first.

## 📄 License

MIT, see [LICENSE](LICENSE). Extracted from [Xapiand](https://github.com/Kronuz/Xapiand).
