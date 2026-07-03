/*
 * IRLSAFETY+ — frozen libirlsafety C API (P9, mobile-ready).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 *
 * Umbrella header for embedders (Android NDK, standalone samples). OBS-specific
 * types live in filter_settings.h; consumers pass irlsafety_filter_settings blobs.
 */

#pragma once

#define IRLSAFETY_API_VERSION 1

#include "custom_pii.h"
#include "hybrid_delay.h"
#include "irlsafety_log.h"
#include "irlsafety_paths.h"
#include "irlsafety_runtime.h"
#include "irlsafety_settings.h"
#include "irlsafety_shutdown.h"
#include "irlsafety_types.h"
#include "pipeline.h"
#include "virtual_cam/virtual_cam.h"

#ifdef __cplusplus
#include "onnx/ort_ep.h"
#endif