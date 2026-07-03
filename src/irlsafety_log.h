/*
 * IRLSAFETY+ — injectable logging for libirlsafety (OBS-agnostic).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IRLSAFETY_LOG_WARNING 2
#define IRLSAFETY_LOG_INFO 3

typedef void (*irlsafety_log_fn)(int level, const char *message, void *userdata);

void irlsafety_log_set_callback(irlsafety_log_fn fn, void *userdata);
void irlsafety_log(int level, const char *format, ...);

#ifdef __cplusplus
}
#endif