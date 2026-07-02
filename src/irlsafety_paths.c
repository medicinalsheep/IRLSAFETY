/*
 * IRLSAFETY+ — bundled model path resolution (OBS module paths).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "irlsafety_paths.h"

#include <string.h>

#ifndef IRLSAFETY_TEST_BUILD
#include <obs-module.h>
#include <util/bmem.h>
#endif

void irlsafety_resolve_model_path(const char *bundled_relative, const char *user_path, char *dest, size_t dest_size)
{
	if (!dest || dest_size == 0)
		return;

	dest[0] = '\0';

	if (user_path && user_path[0] != '\0') {
		strncpy(dest, user_path, dest_size - 1);
		dest[dest_size - 1] = '\0';
		return;
	}

#ifndef IRLSAFETY_TEST_BUILD
	if (bundled_relative && bundled_relative[0] != '\0') {
		char *bundled = obs_module_file(bundled_relative);

		if (bundled) {
			strncpy(dest, bundled, dest_size - 1);
			dest[dest_size - 1] = '\0';
			bfree(bundled);
		}
	}
#else
	(void)bundled_relative;
#endif
}