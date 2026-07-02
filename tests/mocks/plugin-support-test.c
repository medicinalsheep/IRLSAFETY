/*
 * IRLSAFETY+ — plugin-support stub for OBS mock tests.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include <plugin-support.h>

#include <stdarg.h>
#include <stdio.h>

const char *PLUGIN_NAME = "irlsafety-plus";
const char *PLUGIN_VERSION = "0.1.3";

void obs_log(int log_level, const char *format, ...)
{
	(void)log_level;
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}