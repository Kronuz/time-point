# Architecture

The internal design of `time_point`: a single header with a signed and an unsigned pair of
conversion templates around `std::chrono::time_point`, which also re-exports `nanosleep`
from the nanosleep library. For usage see `README.md`; for the repo map and invariants
see `AGENTS.md`.

## Shape

One header, header-only, nothing behind it but the standard `<chrono>` and the C
`<time.h>` / `<errno.h>`:

```
  time_point.hh   time_point <-> llong and ullong conversions; includes nanosleep.h
```

## The conversions

The core is `time_point_to_llong` and `time_point_from_llong`, a pair of inverse function
templates that flatten a `time_point` to a signed 64-bit count of nanoseconds and back.
`time_point_to_ullong` and `time_point_from_ullong` are the same count as its unsigned
two's complement image, one `static_cast` on each side.

`time_point_to_llong(std::chrono::time_point<Clock, Duration> t)` takes the time_point of
any clock and duration, reads `time_since_epoch()`, converts it to `nanoseconds` and
returns the `count()`. The whole instant becomes one integer: nanoseconds since that
clock's epoch, negative before it. The clock and the duration are deduced from the
argument, so the same call works for `steady_clock`, `system_clock`, a
`time_point<system_clock, seconds>`, or any other clock's time_point.

`time_point_from_llong<Clock>(long long t)` goes the other way: it wraps the integer in
`std::chrono::nanoseconds` and converts it to `Clock::time_point`'s own duration. Here the
clock can't be deduced from a plain integer, so it's an explicit template parameter that
defaults to `std::chrono::steady_clock`. Pass the clock you stored the value against if
it wasn't the default.

Both conversions to a different tick use one helper, `time_point_detail::floor_to`. A
lossless conversion (a nanosecond count onto a nanosecond or finer clock, a coarser
duration to nanoseconds) is the implicit `std::chrono` conversion and costs nothing. A
lossy one (a nanosecond count onto libc++'s microsecond `system_clock`, a duration finer
than a nanosecond to nanoseconds) goes through `std::chrono::floor`: the result is the
last tick not after the instant. (`duration_cast` truncates toward zero, which would round
an instant before the epoch to a later tick.)

The round trip is lossless where no tick is dropped. An instant's count and back is exact
on a clock that ticks in nanoseconds or coarser (the count is a whole number of ticks); a
count and back is exact on a clock that ticks in nanoseconds or finer. The standard clocks
tick in nanoseconds, or in microseconds for libc++'s `system_clock`.

## The unsigned pair

`time_point_from_ullong` casts its argument to `long long` (modular, defined in C++20) and
calls `time_point_from_llong`; `time_point_to_ullong` casts the signed count to
`unsigned long long`. For an instant at or after the epoch the unsigned value is the
count itself. An instant before the epoch maps to `2^64` minus its distance, a value of
`2^63` or more, which is how it travels without loss but sorts after every later instant
as an unsigned integer. A caller that stores instants which can precede the epoch uses the
signed pair, whose counts order like the instants.

## nanosleep

`nanosleep(unsigned long long nsec)` comes from the
[nanosleep](https://github.com/Kronuz/nanosleep) library, which documents the EINTR
retry loop. This header includes it so existing callers keep compiling.

## Why a flat integer

Collapsing a time_point to one 64-bit integer is what lets an instant be stored in a
fixed-width column, sent on the wire, or compared as a plain integer, without dragging
the `chrono` type along. The choice of nanoseconds-since-epoch as the unit is what makes
the two directions exact inverses and keeps the signed value ordered the same way the
instants are. That is the contract these helpers exist to provide, preserved here from
Xapiand.
