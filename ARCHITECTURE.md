# Architecture

The internal design of `time_point`: a single header with two conversion
templates around `std::chrono::time_point`, which also re-exports `nanosleep` from
the nanosleep library. For usage see `README.md`; for the repo map and invariants
see `AGENTS.md`.

## Shape

One header, header-only, nothing behind it but the standard `<chrono>` and the C
`<time.h>` / `<errno.h>`:

```
  time_point.hh   two time_point <-> ullong conversions; includes nanosleep.h
```

## The two conversions

The core is a pair of inverse function templates that flatten a `time_point` to a
single integer and back.

`time_point_to_ullong(std::chrono::time_point<T> t)` takes the time_point of any
clock `T`, reads `time_since_epoch()`, `duration_cast`s it to `nanoseconds`, and
returns the raw `count()` as an `unsigned long long`. The whole instant becomes
one integer: nanoseconds since that clock's epoch. The clock is deduced from the
argument, so the same call works for `steady_clock`, `system_clock`, or any other
clock's time_point.

`time_point_from_ullong<T>(unsigned long long t)` goes the other way: it wraps the
integer in `std::chrono::nanoseconds` and constructs a `T::time_point` from it.
Here the clock can't be deduced from a plain integer, so it's an explicit template
parameter that defaults to `std::chrono::steady_clock`. Pass the clock you stored
the value against if it wasn't the default.

The round-trip is lossless at nanosecond resolution because both directions pivot
on `nanoseconds`: `to_ullong` casts down to nanoseconds, `from_ullong` rebuilds
from nanoseconds, so an instant that started life at nanosecond (or coarser)
granularity comes back bit-identical. The standard clocks tick at nanosecond
resolution or finer, so the cast to `nanoseconds` is the only place precision
could be lost, and a value that originated as an integer count of nanoseconds
loses nothing.

## nanosleep

`nanosleep(unsigned long long nsec)` comes from the
[nanosleep](https://github.com/Kronuz/nanosleep) library, which documents the EINTR
retry loop. This header includes it so existing callers keep compiling.

## Why a flat integer

Collapsing a time_point to one `unsigned long long` is what lets an instant be
stored in a fixed-width column, sent on the wire, or compared as a plain integer,
without dragging the `chrono` type along. The choice of nanoseconds-since-epoch as
the unit is what makes the two directions exact inverses and keeps the value
ordered the same way the instants are. That is the contract these helpers exist to
provide, preserved here from Xapiand.
