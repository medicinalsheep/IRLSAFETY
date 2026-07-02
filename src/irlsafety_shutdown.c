/*
 * IRLSAFETY+ — graceful shutdown coordination (OBS exit / module unload).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "irlsafety_shutdown.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

static volatile LONG g_irlsafety_shutting_down;

void irlsafety_begin_shutdown(void)
{
	InterlockedExchange(&g_irlsafety_shutting_down, 1);
}

bool irlsafety_is_shutting_down(void)
{
	return InterlockedCompareExchange(&g_irlsafety_shutting_down, 0, 0) != 0;
}
#else
static volatile int g_irlsafety_shutting_down;

void irlsafety_begin_shutdown(void)
{
	g_irlsafety_shutting_down = 1;
}

bool irlsafety_is_shutting_down(void)
{
	return g_irlsafety_shutting_down != 0;
}
#endif