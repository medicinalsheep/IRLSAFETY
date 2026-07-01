/*
 * IRLSAFETY+ — OBS video filter for PII blurring.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#ifdef IRLSAFETY_TEST_BUILD
#include "obs-mock.h"
#else
#include <obs-module.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern struct obs_source_info irlsafety_pii_filter;

#ifdef __cplusplus
}
#endif