/*
 * IRLSAFETY+ — OBS platform adapter for libirlsafety.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void irlsafety_obs_adapter_register(void);
void irlsafety_obs_adapter_unregister(void);

void irlsafety_obs_on_stream_started(void);
void irlsafety_obs_on_stream_stopped(void);

#ifdef __cplusplus
}
#endif