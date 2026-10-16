# AGENTS.md

Working notes for agents modifying this repository. For the design read
`ARCHITECTURE.md`; for usage read `README.md`. This file covers the repo layout,
how to build and test, the invariants you must not break, and the traps that are
easy to fall into.

## Repo map

```
time_point.hh                Two time_point <-> ullong conversions (time_point_to_ullong / time_point_from_ullong); re-exports nanosleep.h. Header.
test/test.cc                 Runnable smoke test: round-trips both ways, now() monotonicity, nanosleep advances the clock.
test/both_headers.cc         time_point.hh and nanosleep.h in one TU (the redefinition regression).
CMakeLists.txt               INTERFACE library `time_point` (+ alias time_point::time_point); CTest test `time_point`.
LICENSE                      MIT, Copyright (c) 2015-2019 Dubalu LLC.
README.md                    What it is, install, usage.
ARCHITECTURE.md              Internal design: the two conversions.
```

Everything is header-only. There is no `.cc` to compile except the test. The
CMake target is a pure `INTERFACE` library that only adds the source dir to the
include path and requests `cxx_std_20`.

## Build and run the test

```sh
cmake -B build && cmake --build build && ctest --test-dir build
```

Expected output ends with `all time_point tests passed`, exit 0. The test target
is `time_point_test`; the registered CTest name is `time_point`.

## Conventions

- **C++20.** The target requests `cxx_std_20` to stay uniform with the sibling
  libraries; the header itself is older-standard-friendly, but don't drop the
  target below it.
- **One dependency: nanosleep.** Besides the standard `<chrono>`, `<cmath>`, and
  `<errno.h>`, the header includes `nanosleep.h` (fetched by CMake). Don't add
  anything else, and in particular don't pull a local Xapiand header back in.
- **Filename is stable.** The header keeps its original Xapiand name
  (`time_point.hh`) so a consumer that already `#include`s it just needs this repo
  on the include path. Don't rename it.
- Tabs for indentation, double quotes in code, no em dashes in prose.

## Load-bearing invariants

- **The two conversions are inverses at nanosecond resolution.** `to_ullong`
  casts the time_point's `time_since_epoch()` down to `nanoseconds` and returns
  the count; `from_ullong` rebuilds a `time_point` from those same nanoseconds.
  The pivot unit is nanoseconds on both sides; don't change one direction's unit
  without changing the other, or the round-trip stops being exact.
- **`time_point_from_ullong`'s clock defaults to `steady_clock`.** The clock is a
  template parameter because it can't be deduced from a plain integer. Keep the
  default so existing call sites that relied on it don't break, and remember the
  value is only meaningful paired with the clock it was produced against.
- **`nanosleep` lives in the nanosleep library.** This header includes
  `nanosleep.h` instead of defining its own copy; a second definition here breaks
  any file that includes both. Change the wrapper there.

## How to extend

- **Add a helper.** Mirror the existing pattern: a small free function template
  over a clock `T`, pivoting on `nanoseconds` so it composes with the existing
  conversions and stays exact. Keep the surface free functions, not a class.
- **Always extend the smoke test.** `test/test.cc` is the only executable check.
  It round-trips known integers, starts from a live `now()` the other way, checks
  `steady_clock` monotonicity, and times a real sleep against an independent clock
  read. Keep that independence (don't verify a conversion with itself) when you
  add cases.

## Traps

- **Nanosecond range.** `unsigned long long` of nanoseconds since epoch is wide
  (hundreds of years), but it is still finite; don't assume it can't overflow for
  a far-future `system_clock` instant.
- **Clock mismatch.** A value produced from a `steady_clock` time_point is
  nanoseconds since the *steady* epoch, which is arbitrary (often boot time), not
  wall-clock time. Convert it back with the same clock; don't feed a steady value
  into `from_ullong<system_clock>` and expect a calendar time.
- **The `<cmath>` include is inherited from Xapiand.** The current helpers don't
  use `std::pow`/`std::floor`/`std::log10`; the include rides along verbatim.
  Leave it unless you're deliberately trimming, to keep the file reconcilable with
  upstream.

## Standalone vs. Xapiand

This is a standalone extraction from
[Xapiand](https://github.com/Kronuz/Xapiand). `time_point.hh` was copied verbatim,
except that its nanosleep wrapper now comes from the nanosleep library, which
Xapiand's copy duplicated.
