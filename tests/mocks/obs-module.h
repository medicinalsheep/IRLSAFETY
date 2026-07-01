/*
 * IRLSAFETY+ — OBS module header shim for unit tests.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include "obs-mock.h"

void obs_log(int log_level, const char *format, ...);