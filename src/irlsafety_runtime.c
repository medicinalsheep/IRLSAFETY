/*
 * IRLSAFETY+ — portable runtime hooks.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "irlsafety_runtime.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#else
#include <time.h>
#include <unistd.h>
#endif

void irlsafety_sleep_ms(unsigned ms)
{
#ifdef _WIN32
	Sleep(ms);
#else
	if (ms > 0)
		usleep((useconds_t)ms * 1000U);
#endif
}

uint64_t irlsafety_monotonic_ms(void)
{
#ifdef _WIN32
	return (uint64_t)GetTickCount64();
#else
	struct timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
		return 0;

	return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
#endif
}