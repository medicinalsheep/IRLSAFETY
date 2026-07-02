/*
 * IRLSAFETY+ — graceful shutdown coordination (OBS exit / module unload).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void irlsafety_begin_shutdown(void);
bool irlsafety_is_shutting_down(void);

#ifdef __cplusplus
}
#endif