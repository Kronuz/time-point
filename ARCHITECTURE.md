# Architecture

The internal design of `time_point`: a single header with two conversion
templates around `std::chrono::time_point` and a small `nanosleep` wrapper. For
usage see `README.md`; for the repo map and invariants see `AGENTS.md`.

## Shape

One header, header-only, nothing behind it but the standard `<chrono>` and the C
`<time.h>` / `<errno.h>`:

```
  time_point.hh   two time_point <-> ullong conversions + a nanosleep wrapper
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

## The nanosleep wrapper

The header also defines a free `nanosleep(unsigned long long nsec)` that takes a
flat nanosecond count rather than a `struct timespec`. It splits the count into
seconds and the nanosecond remainder, fills a `timespec`, and loops on the POSIX
`::nanosleep` while it returns `< 0` with `errno == EINTR`, so a signal
interrupting the sleep resumes it with the remaining time the kernel writes back
into the same `timespec`. A zero count short-circuits to a no-op before touching
the clock at all.

The wrapper shares its name with the POSIX `nanosleep`; inside its body the call
`nanosleep(&ts, &ts)` still resolves to the two-argument C function (different
signature), so the one-argument overload and the system call coexist. That is the
original Xapiand shape, kept as-is.

## Why a flat integer

Collapsing a time_point to one `unsigned long long` is what lets an instant be
stored in a fixed-width column, sent on the wire, or compared as a plain integer,
without dragging the `chrono` type along. The choice of nanoseconds-since-epoch as
the unit is what makes the two directions exact inverses and keeps the value
ordered the same way the instants are. That is the contract these helpers exist to
provide, preserved here from Xapiand.
