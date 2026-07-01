/*
 * IRLSAFETY+ — OBS plugin module entry.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include <obs-module.h>
#include <plugin-support.h>

#include "pii-filter.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

bool obs_module_load(void)
{
	obs_register_source(&irlsafety_pii_filter);
	obs_log(LOG_INFO, "IRLSAFETY+ loaded (version %s) — PII blur filter registered", PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	obs_log(LOG_INFO, "IRLSAFETY+ unloaded");
}