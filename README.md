# time-point

Part of [Kronuz C++ Libraries](https://github.com/Kronuz/cpp-libraries), a collection of small, reusable C++ libraries.

[![CI](https://github.com/Kronuz/time-point/actions/workflows/ci.yml/badge.svg)](https://github.com/Kronuz/time-point/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![Header-only](https://img.shields.io/badge/header--only-yes-green.svg)

Pack a `std::chrono::time_point` into one integer of nanoseconds and back, in
C++20. For storing or sending an instant as a single field.

## ✨ Highlights

- **`time_point_to_ullong(tp)`**: nanoseconds since that clock's epoch.
- **`time_point_from_ullong<Clock>(n)`**: the same instant back, on `Clock`
  (`steady_clock` by default).
- **Exact round trips** at nanosecond resolution, for any standard clock.

## 📦 Installation

Requires a C++20 compiler; CMake 3.14 or newer for the CMake route. CI builds it
on Linux (x86_64, arm64), macOS (arm64, x86_64), and FreeBSD, plus ASan, UBSan,
and TSan builds.

```cmake
include(FetchContent)
FetchContent_Declare(time_point
  GIT_REPOSITORY https://github.com/Kronuz/time-point.git
  GIT_TAG        main)
FetchContent_MakeAvailable(time_point)

target_link_libraries(my_app PRIVATE time_point::time_point)
```

## 🚀 Quick start

```cpp
#include "time_point.hh"

#include <cassert>
#include <chrono>

int main() {
	auto now = std::chrono::steady_clock::now();
	unsigned long long packed = time_point_to_ullong(now);
	assert(time_point_from_ullong(packed) == now);
}
```

Convert back on the clock the value came from: each clock has its own epoch.

## 🎯 When to use it

- ✅ Timestamps in binary records, queues, and atomics.
- ℹ️ The header re-exports `nanosleep(nsec)` from the
  [nanosleep](https://github.com/Kronuz/nanosleep) library, which CMake fetches.

## 📚 Documentation

- [ARCHITECTURE.md](ARCHITECTURE.md): the conversions.
- [AGENTS.md](AGENTS.md): repository map and traps.

## 🛠️ Building and testing

```sh
cmake -B build && cmake --build build && ctest --test-dir build
```

## 🤝 Contributing

Issues and pull requests are welcome. Read [AGENTS.md](AGENTS.md) first. CI must
pass on every platform.

## 📄 License

MIT, Copyright (c) 2015-2018 Dubalu LLC. See [LICENSE](LICENSE). Extracted from
[Xapiand](https://github.com/Kronuz/Xapiand).
