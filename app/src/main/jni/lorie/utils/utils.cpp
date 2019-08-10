#include <cstdint>
#include <lorie-compositor.hpp>
#include <time.h>

uint32_t LorieUtils::timestamp() {
	struct timespec t;
	clock_gettime (CLOCK_MONOTONIC, &t);
	return t.tv_sec * 1000 + t.tv_nsec / 1000000;
}
