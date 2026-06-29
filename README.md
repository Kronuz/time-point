# time_point

A small, header-only, dependency-free **`std::chrono::time_point` helpers**
library for C++20, extracted from [Xapiand](https://github.com/Kronuz/Xapiand).

## What it is

One header, `time_point.hh`: a couple of free function templates that move a
`std::chrono::time_point` to and from a flat unsigned integer (nanoseconds since
the clock's epoch), plus a tiny `nanosleep` wrapper that takes a count in
nanoseconds. Nothing behind it but the standard `<chrono>` and the C `<time.h>` /
`<errno.h>`.

The full set:

- **`time_point_to_ullong(time_point<T>)`** — collapse any clock's time_point to
  an `unsigned long long` of nanoseconds since that clock's epoch. The natural
  shape for storing or sending an instant as a single integer.
- **`time_point_from_ullong<T>(unsigned long long)`** — rebuild a `time_point` on
  clock `T` from that integer. `T` defaults to `std::chrono::steady_clock`.
- **`nanosleep(unsigned long long nsec)`** — sleep for `nsec` nanoseconds,
  retrying across `EINTR`. A zero count is a no-op.

The two conversions are inverses at nanosecond resolution: `to_ullong` then
`from_ullong` (or the other way) lands back on the same instant, for any of the
standard clocks.

## Install

CMake with `FetchContent`:

```cmake
include(FetchContent)
FetchContent_Declare(
  time_point
  GIT_REPOSITORY https://github.com/Kronuz/time-point.git
  GIT_TAG        main
)
FetchContent_MakeAvailable(time_point)

target_link_libraries(your_target PRIVATE time_point::time_point)
```

The `time_point` target is a pure `INTERFACE` library: it compiles nothing,
requests `cxx_std_20`, and puts the header directory on your include path. Then:

```cpp
#include "time_point.hh"
```

Requires C++20. On macOS it builds with AppleClang/libc++, the same toolchain
Xapiand uses. The header keeps its original filename, so a codebase that already
`#include "time_point.hh"` just needs this repo on its include path.

## Usage

```cpp
#include "time_point.hh"
#include <chrono>

// Store an instant as a single integer, restore it later:
auto now  = std::chrono::steady_clock::now();
unsigned long long packed = time_point_to_ullong(now);   // ns since epoch
auto back = time_point_from_ullong(packed);              // back == now

// Pick the clock explicitly when it matters:
auto sys  = time_point_from_ullong<std::chrono::system_clock>(packed);

// Sleep for a count of nanoseconds, EINTR-safe:
nanosleep(5'000'000ULL);   // 5 milliseconds
nanosleep(0);              // no-op
```

`time_point_to_ullong` works on any clock's time_point; the value it returns is
nanoseconds since *that clock's* epoch, so pair it with the matching clock when
you convert back.

## Build & test

```sh
cmake -B build && cmake --build build && ctest --test-dir build
```

The test round-trips integers through `from_ullong` / `to_ullong` (and back the
other way starting from a live `now()`), checks `steady_clock::now()` is non-zero
and monotonic non-decreasing across two reads, and confirms the `nanosleep`
wrapper actually passes time (a 5ms sleep advances an independent `steady_clock`
read). It prints `all time_point tests passed` and exits 0.

## Provenance

Extracted from [Xapiand](https://github.com/Kronuz/Xapiand). `time_point.hh` had
zero dependencies already (only `<chrono>` and the C time/errno headers), so it
was copied verbatim, no decoupling delta. See [ARCHITECTURE.md](ARCHITECTURE.md)
for the design and [AGENTS.md](AGENTS.md) for the repo map and invariants.

## License

MIT, Copyright (c) 2015-2019 Dubalu LLC. See [LICENSE](LICENSE).
