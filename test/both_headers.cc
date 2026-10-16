// Including time_point.hh and nanosleep.h in one translation unit must compile: there is
// one nanosleep(unsigned long long), owned by the nanosleep library.
#include "nanosleep.h"
#include "time_point.hh"

int main() {
	nanosleep(0ULL);
	return time_point_to_ullong(std::chrono::steady_clock::time_point{}) == 0 ? 0 : 1;
}
