# AGENTS.md

Working notes for agents modifying this repository. For the design read
`ARCHITECTURE.md`; for usage read `README.md`. This file covers the repo layout,
how to build and test, the invariants you must not break, and the traps that are
easy to fall into.

## Repo map

```
time_point.hh                time_point <-> llong / ullong conversions (time_point_to_llong / _from_llong, time_point_to_ullong / _from_ullong); re-exports nanosleep.h. Header.
test/test.cc                 Runnable smoke test: round-trips both ways, now() monotonicity, nanosleep advances the clock.
test/both_headers.cc         time_point.hh and nanosleep.h in one TU (the redefinition regression).
test/coarse_clocks.cc        Clocks that tick coarser or finer than a nanosecond, and the real system_clock, through the unsigned pair, against std::chrono comparisons.
test/pre_epoch.cc            The signed pair: edges, order of the counts, round trips, instants before the epoch, floor on coarse and fine clocks.
test/diff.cc                 The library against the frozen old conversions (test/old/); the one result class that differs is listed and asserted.
test/clocks.h, check.h       Test clocks with a chosen tick; CHECK(), which stays live under NDEBUG.
CMakeLists.txt               INTERFACE library `time_point` (+ alias time_point::time_point); CTest tests `time_point`, `time_point_both_headers`, `time_point_coarse_clocks`, `time_point_pre_epoch`, `time_point_diff`.
LICENSE                      MIT, Copyright (c) 2015-2019 Dubalu LLC.
README.md                    What it is, install, usage.
ARCHITECTURE.md              Internal design: the conversions.
CHANGELOG.md                 What changed, dated.
```

Everything is header-only. There is no `.cc` to compile except the test. The
CMake target is a pure `INTERFACE` library that only adds the source dir to the
include path and requests `cxx_std_20`.

## Build and run the test

```sh
cmake -B build && cmake --build build && ctest --test-dir build
```

Expected output ends with `all time_point tests passed`, exit 0. The test target
is `time_point_test`; the registered CTest names are `time_point`,
`time_point_both_headers`, `time_point_coarse_clocks`, `time_point_pre_epoch` and
`time_point_diff`. `test/test.cc` undefines `NDEBUG` so its `assert`s run in a Release
build; the other tests use `CHECK`.

## Conventions

- **C++20.** The target requests `cxx_std_20` to stay uniform with the sibling
  libraries; the header itself is older-standard-friendly, but don't drop the
  target below it.
- **One dependency: nanosleep.** Besides the standard `<chrono>`, `<cmath>`,
  `<errno.h>` and `<type_traits>`, the header includes `nanosleep.h` (fetched by CMake).
  Don't add anything else, and in particular don't pull a local Xapiand header back in.
- **Filename is stable.** The header keeps its original Xapiand name
  (`time_point.hh`) so a consumer that already `#include`s it just needs this repo
  on the include path. Don't rename it.
- Tabs for indentation, double quotes in code, no em dashes in prose.

## Load-bearing invariants

- **The signed pair is the count; the unsigned pair is its image.**
  `time_point_to_llong` floors the clock's `time_since_epoch()` to `nanoseconds` and
  returns the count; `time_point_from_llong` converts those nanoseconds to the clock's
  own duration, flooring when the clock ticks coarser. `time_point_to_ullong` and
  `time_point_from_ullong` are one `static_cast` over them, so an instant before the
  epoch is a value of `2^63` and up. Don't give the unsigned pair its own arithmetic: the
  two must agree on every instant.
- **A lossy conversion floors, never truncates.** `time_point_detail::floor_to` uses the
  implicit `std::chrono` conversion when it is lossless and `std::chrono::floor`
  otherwise. `duration_cast` rounds toward zero, which moves an instant before the epoch
  to a later tick.
- **`time_point_from_ullong`'s clock defaults to `steady_clock`.** The clock is a
  template parameter because it can't be deduced from a plain integer. Keep the
  default so existing call sites that relied on it don't break, and remember the
  value is only meaningful paired with the clock it was produced against.
- **`nanosleep` lives in the nanosleep library.** This header includes
  `nanosleep.h` instead of defining its own copy; a second definition here breaks
  any file that includes both. Change the wrapper there.

## How to extend

- **Add a helper.** Mirror the existing pattern: a small free function template
  over a clock and duration, pivoting on `nanoseconds` so it composes with the
  existing conversions and stays exact. Keep the surface free functions, not a class.
- **Always extend the tests.** `test/test.cc` round-trips known integers, starts from a
  live `now()` the other way, checks `steady_clock` monotonicity, and times a real
  sleep against an independent clock read. The coarse-clock and pre-epoch tests check
  every result against `std::chrono` comparisons (the count of a tick, and
  `0 <= n - tick start < one tick`), never against a second copy of the division. Keep
  that independence (don't verify a conversion with itself) when you add cases.
  `test/old/time_point_old.hh` is the frozen original; leave it as it is.

## Traps

- **Nanosecond range.** A count of nanoseconds since the epoch holds about 292 years
  either side of it (`long long`); a far-future `system_clock` instant, or a
  `time_point` with a wide duration (seconds in the year 3000), overflows it.
- **Clock mismatch.** A value produced from a `steady_clock` time_point is
  nanoseconds since the *steady* epoch, which is arbitrary (often boot time), not
  wall-clock time. Convert it back with the same clock; don't feed a steady value
  into `from_ullong<system_clock>` and expect a calendar time.
- **Unsigned order across the epoch.** An instant before the epoch is `2^63` or more as
  an `unsigned long long`; compare and sort the signed count.
- **A coarse clock drops the nanoseconds.** `time_point_from_llong<system_clock>` on libc++
  returns the microsecond at or before the count, so the count read back is smaller by up
  to 999 ns.
- **The `<cmath>` include is inherited from Xapiand.** The current helpers don't
  use `std::pow`/`std::floor`/`std::log10`; the include rides along verbatim.
  Leave it unless you're deliberately trimming, to keep the file reconcilable with
  upstream.

## Standalone vs. Xapiand

This is a standalone extraction from
[Xapiand](https://github.com/Kronuz/Xapiand). `time_point.hh` was copied from it, except
that its nanosleep wrapper comes from the nanosleep library, which Xapiand's copy
duplicated, and its conversions floor and have a signed pair (see `CHANGELOG.md`).
