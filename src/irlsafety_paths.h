/*
 * IRLSAFETY+ — bundled asset path resolution (platform-injectable).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Resolve a bundled relative path; returns heap string or NULL. Caller frees via irlsafety_paths_free_string. */
typedef char *(*irlsafety_bundled_path_fn)(const char *relative, void *userdata);

void irlsafety_paths_set_resolver(irlsafety_bundled_path_fn fn, void *userdata);
void irlsafety_paths_free_string(char *path);

/* Prefer user_path when set; otherwise resolve bundled relative path via registered resolver. */
void irlsafety_resolve_model_path(const char *bundled_relative, const char *user_path, char *dest, size_t dest_size);

#ifdef __cplusplus
}
#endif