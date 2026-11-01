# time-point

Part of [Kronuz C++ Libraries](https://github.com/Kronuz/cpp-libraries), a collection of small, reusable C++ libraries.

[![CI](https://github.com/Kronuz/time-point/actions/workflows/ci.yml/badge.svg)](https://github.com/Kronuz/time-point/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![Header-only](https://img.shields.io/badge/header--only-yes-green.svg)

Header-only time_point packing for C++20: a std::chrono instant to one integer of
nanoseconds and back. Both directions pivot on `std::chrono::nanoseconds`, so the value
is the count since the clock's epoch and a round trip is exact. Reach for it to store an
instant in a fixed-width column, a binary record, a queue entry or an atomic.

## ✨ Highlights

- **`time_point_to_ullong(tp)`:** nanoseconds since the epoch of `tp`'s clock, as an
  `unsigned long long`; the clock is deduced.
- **`time_point_from_ullong<Clock>(n)`:** the same instant back on `Clock`, which
  defaults to `std::chrono::steady_clock`.
- **Exact round trips** for clocks that tick in nanoseconds.
- **Two function templates** in one header, over `<chrono>`.

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
  GIT_TAG        2f1f7f942090d2941f970390dcad56c9fe99ccaa)  # or main, to track the tip
FetchContent_MakeAvailable(time_point)

target_link_libraries(my_app PRIVATE time_point::time_point)
```

Link `time_point::time_point` and include `time_point.hh`.

## 🎯 When to use it

- Timestamps in binary records, wire formats, queues and atomics, where one integer
  travels better than a `std::chrono` type.

Not for:

- Clocks that tick coarser than nanoseconds on the way back:
  `time_point_from_ullong<std::chrono::system_clock>` does not compile on libc++, whose
  `system_clock` ticks in microseconds. `time_point_to_ullong` takes any clock.
- Instants before the clock's epoch: the count is unsigned, so a negative one wraps to a
  huge value and sorts after every later instant.
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
