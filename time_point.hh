/*
 * Copyright © 2026 Germán Méndez Bravo (Kronuz)
 * Copyright (c) 2015-2019 Dubalu LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma once

#include <chrono>            // for std::chrono
#include <cmath>             // for std::pow, std::floor, std::log10
#include <errno.h>           // for errno
#include <type_traits>       // for std::is_convertible_v
#include "nanosleep.h"       // for nanosleep(unsigned long long), re-exported


namespace time_point_detail {

// The largest `To` not after `d`. A lossless conversion is the implicit one; otherwise the
// count rounds toward negative infinity, so an instant before the epoch never rounds later.
template <typename To, typename Rep, typename Period>
constexpr To
floor_to(std::chrono::duration<Rep, Period> d) {
	if constexpr (std::is_convertible_v<std::chrono::duration<Rep, Period>, To>) {
		return To(d);
	} else {
		return std::chrono::floor<To>(d);
	}
}

}  // namespace time_point_detail


// Nanoseconds since the clock's epoch, negative before it. A tick finer than a nanosecond
// rounds down to the nanosecond.
template <typename Clock, typename Duration>
inline long long
time_point_to_llong(std::chrono::time_point<Clock, Duration> t) {
	return time_point_detail::floor_to<std::chrono::nanoseconds>(t.time_since_epoch()).count();
}


// The instant `t` nanoseconds after the clock's epoch (before it when negative). On a clock
// whose tick is coarser than a nanosecond the instant rounds down to the tick.
template <typename Clock = std::chrono::steady_clock>
inline std::chrono::time_point<Clock>
time_point_from_llong(long long t) {
	using time_point = typename Clock::time_point;
	return time_point(time_point_detail::floor_to<typename time_point::duration>(std::chrono::nanoseconds(t)));
}


// The same count as time_point_to_llong, as its unsigned two's complement image: an instant
// before the epoch is a value of 2^63 and up.
template <typename Clock, typename Duration>
inline unsigned long long
time_point_to_ullong(std::chrono::time_point<Clock, Duration> t) {
	return static_cast<unsigned long long>(time_point_to_llong(t));
}


// The inverse of time_point_to_ullong: a value of 2^63 and up is an instant before the epoch.
template <typename Clock = std::chrono::steady_clock>
inline std::chrono::time_point<Clock>
time_point_from_ullong(unsigned long long t) {
	return time_point_from_llong<Clock>(static_cast<long long>(t));
}
