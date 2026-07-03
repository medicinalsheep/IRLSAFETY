/*
 * IRLSAFETY+ — portable runtime hooks (sleep, etc.).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void irlsafety_sleep_ms(unsigned ms);
uint64_t irlsafety_monotonic_ms(void);

#ifdef __cplusplus
}
#endif