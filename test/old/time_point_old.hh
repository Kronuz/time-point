// The two conversions exactly as time_point.hh had them before the signed pair and the
// coarse-clock fix (43d8a84), in namespace time_point_old. Frozen: the differential test
// in test/diff.cc compares the library against it. Do not edit.
#pragma once

#include <chrono>

namespace time_point_old {

template <typename T>
inline unsigned long long
time_point_to_ullong(std::chrono::time_point<T> t) {
	return std::chrono::duration_cast<std::chrono::nanoseconds>(t.time_since_epoch()).count();
}


template <typename T = std::chrono::steady_clock>
inline std::chrono::time_point<T>
time_point_from_ullong(unsigned long long t) {
	return typename T::time_point(std::chrono::nanoseconds(t));
}

}  // namespace time_point_old
