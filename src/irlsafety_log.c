/*
 * IRLSAFETY+ — injectable logging for libirlsafety.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "irlsafety_log.h"

#include <stdio.h>
#include <string.h>

static irlsafety_log_fn g_log_fn;
static void *g_log_userdata;

void irlsafety_log_set_callback(irlsafety_log_fn fn, void *userdata)
{
	g_log_fn = fn;
	g_log_userdata = userdata;
}

void irlsafety_log(int level, const char *format, ...)
{
	char buffer[1024];
	va_list args;

	if (!format || !g_log_fn)
		return;

	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	buffer[sizeof(buffer) - 1] = '\0';

	g_log_fn(level, buffer, g_log_userdata);
}