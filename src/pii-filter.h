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

#define IRLSAFETY_PII_FILTER_ID "irlsafety_plus_pii_blur"

struct irlsafety_runtime_status;

int irlsafety_filter_get_runtime_status(obs_source_t *filter, struct irlsafety_runtime_status *out);
size_t irlsafety_filter_copy_censor_log(obs_source_t *filter, struct irlsafety_censor_log_entry *out, size_t max_entries);
void irlsafety_filter_clear_censor_log(obs_source_t *filter);
int irlsafety_filter_reload_model(obs_source_t *filter);

#ifdef __cplusplus
}
#endif