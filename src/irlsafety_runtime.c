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