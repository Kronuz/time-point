# Changelog

All notable changes to `time-point`. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/); the library has no
releases, so entries are dated.

Upgrading a pin from before 2026-10-30: [the consumer upgrade note](https://github.com/Kronuz/cpp-libraries/blob/main/docs/UPGRADING-2026-10.md#time-point) lists what changes for callers, how to find affected inputs, the commits not to pin, and the speedup.

## 2026-11-01

### Fixed

- `time_point_from_ullong<Clock>` builds for a clock that ticks coarser than a
  nanosecond. It built `Clock::time_point(std::chrono::nanoseconds(t))`, which
  `std::chrono` rejects when the clock's tick is coarser, so
  `time_point_from_ullong<std::chrono::system_clock>` did not compile on libc++,
  whose `system_clock` ticks in microseconds (libstdc++'s ticks in nanoseconds and
  compiled). The count now converts to the clock's own duration and rounds down to the
  tick: 1719662400123456789 ns is 1719662400123456 µs. The same holds for any clock, so a
  seconds, milliseconds or 1.5 s tick builds on every standard library.
- `time_point_to_ullong` deduces a `time_point<Clock, Duration>` with any duration,
  where it deduced only `time_point<Clock>` (the clock's own duration), so
  `time_point<steady_clock, seconds>` did not compile. A call that named the clock,
  `time_point_to_ullong<steady_clock>(tp)`, still works.

### Added

- `time_point_to_llong` and `time_point_from_llong`, the same nanosecond count as a
  signed `long long`. An instant before the clock's epoch is negative and the counts
  order like the instants. `time_point_to_ullong` and `time_point_from_ullong` are the
  two's complement image of them, as they always were: an instant before the epoch is a
  value of `2^63` or more, which sorts after every later instant as an unsigned integer,
  and the round trip through it is exact. The unsigned values of every instant on a
  nanosecond clock are the values they had.
- `test/coarse_clocks.cc` (`time_point_coarse_clocks`): clocks with a 1 µs, 1 ms, 1 s,
  1 min and 1.5 s tick, a picosecond clock and a floating-point clock, plus the real
  `system_clock`, `steady_clock` and `high_resolution_clock`, through the unsigned pair.
  The oracle is `std::chrono` comparisons (`0 <= n - tick start < one tick`), with the
  `int64` limits as hand-worked decimal literals. Against the old header it fails to
  build: on libstdc++ with the custom coarse clocks, on libc++ also with `system_clock`.
- `test/pre_epoch.cc` (`time_point_pre_epoch`): the signed pair, every pair of instants
  within 1000 ns and 1000 µs of the epoch (the counts order like the instants; unsigned
  order fails exactly across the epoch), 100,000 random instants sorted, 600,000 round
  trips, a 1969 `system_clock` instant, and the signed floor on coarse and fine clocks.
- `test/diff.cc` (`time_point_diff`): the library against the frozen old conversions in
  `test/old/`. On the nanosecond clocks (`steady_clock`, `high_resolution_clock`, a
  test clock), 200,000 random counts per clock and the edge values (0, 1, `2^63`,
  `ULLONG_MAX`, ...) give identical results, bit for bit, in both directions. The results
  differ on one class only, listed in the test: an instant before the
  epoch on a clock finer than a nanosecond, at a tick that is not a whole nanosecond
  (-1500 ps), which the old code truncated toward the epoch (-1 ns) and the new code
  rounds down (-2 ns). The test asserts old != new there and new == `std::chrono`'s
  `0 <= tick - n ns < 1 ns`.
- The tests that use `assert` keep it live under `NDEBUG`, so the README's Release build
  runs them; the new tests use their own `CHECK`.
- `bench/`: the benchmark rows of the Performance section and `abcmp.py`, behind
  `TIME_POINT_BUILD_BENCH`. CI builds them and runs each binary once.

### Changed

- A conversion that must drop precision rounds toward negative infinity
  (`std::chrono::floor`) instead of toward zero (`duration_cast`). Only an instant before
  the epoch can tell them apart; after the epoch they are the same value.

### Performance

Apple M4 Pro (arm64) and Intel i9-9980HK (x86_64), Apple clang 17, `-O2`, 4096 inputs per
row, best of 9 runs of about 15 ms, best of 5 rounds, each binary run twice (the second copy
of the old code, the A/A column, differs by at most about 3%), under an exclusive hold. The
inputs are read through an opaque pointer so a row cannot be hoisted out of its repetitions.
Nanoseconds per operation:

| row | M4 old | M4 new | ratio | i9 old | i9 new | ratio |
|---|---|---|---|---|---|---|
| `time_point_to_ullong(steady tp)` | 0.061 | 0.061 | 1.00 | 0.069 | 0.068 | 0.99 |
| `time_point_from_ullong<steady>(n)` | 0.060 | 0.061 | 1.02 | 0.066 | 0.065 | 0.99 |
| `time_point_to_ullong(system tp)` | 0.164 | 0.162 | 0.99 | 0.301 | 0.298 | 0.99 |
| `from_ullong(to_ullong(steady tp))` | 0.061 | 0.061 | 1.00 | 0.069 | 0.069 | 1.00 |
| `time_point_to_ullong(system_clock::now())` | 14.04 | 13.99 | 1.00 | 27.15 | 27.23 | 1.00 |
| `time_point_from_ullong<system_clock>(n)` | does not build on libc++ | 0.286 | | does not build | 0.972 | |
| `time_point_to_llong(steady tp)` | | 0.061 | | | 0.069 | |
| `time_point_from_llong<steady>(n)` | | 0.061 | | | 0.068 | |

Every row that existed before is within the A/A spread. The rows are `bench/bench.cc`; build it
with `-DTIME_POINT_BUILD_BENCH=ON` and run `python3 bench/abcmp.py build 5` (`AGENTS.md`,
"Verification and benchmarks").
