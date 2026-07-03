/*
 * IRLSAFETY+ — bundled asset path resolution.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "irlsafety_paths.h"

#include <stdlib.h>
#include <string.h>

static irlsafety_bundled_path_fn g_resolver;
static void *g_resolver_userdata;

void irlsafety_paths_set_resolver(irlsafety_bundled_path_fn fn, void *userdata)
{
	g_resolver = fn;
	g_resolver_userdata = userdata;
}

void irlsafety_paths_free_string(char *path)
{
	free(path);
}

void irlsafety_resolve_model_path(const char *bundled_relative, const char *user_path, char *dest, size_t dest_size)
{
	char *bundled;

	if (!dest || dest_size == 0)
		return;

	dest[0] = '\0';

	if (user_path && user_path[0] != '\0') {
		strncpy(dest, user_path, dest_size - 1);
		dest[dest_size - 1] = '\0';
		return;
	}

	if (!bundled_relative || bundled_relative[0] == '\0' || !g_resolver)
		return;

	bundled = g_resolver(bundled_relative, g_resolver_userdata);
	if (!bundled)
		return;

	strncpy(dest, bundled, dest_size - 1);
	dest[dest_size - 1] = '\0';
	irlsafety_paths_free_string(bundled);
}