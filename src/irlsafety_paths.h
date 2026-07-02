/*
 * IRLSAFETY+ — bundled model path resolution (OBS module paths).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Prefer user_path when set; otherwise resolve bundled relative path via obs_module_file. */
void irlsafety_resolve_model_path(const char *bundled_relative, const char *user_path, char *dest, size_t dest_size);

#ifdef __cplusplus
}
#endif