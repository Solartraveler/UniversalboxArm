#include <stdint.h>
#include <time.h>

#include "counter.h"

uint64_t g_stampStart;

void CounterStart(void) {
	struct timespec t = {0};
	clock_gettime(CLOCK_MONOTONIC, &t);
	g_stampStart = (t.tv_sec * 1000) + (t.tv_nsec / 1000000);
}

uint32_t CounterGet(void) {
	struct timespec t = {0};
	clock_gettime(CLOCK_MONOTONIC, &t);
	uint64_t curr = (t.tv_sec * 1000) + (t.tv_nsec / 1000000);
	uint64_t delta = curr - g_stampStart;
	return delta;
}
