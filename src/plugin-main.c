/*
 * IRLSAFETY+ — OBS plugin module entry.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include <obs-module.h>
#include <plugin-support.h>

#include "pii-filter.h"

#if defined(IRLSAFETY_HAS_CONTROL_DOCK)
void irlsafety_control_dock_register(void);
void irlsafety_control_dock_unregister(void);
#endif

#if defined(IRLSAFETY_HAS_FRONTEND_API) && !defined(IRLSAFETY_TEST_BUILD)
#include <obs-frontend-api.h>
#include "hybrid_delay.h"

static void irlsafety_frontend_event(enum obs_frontend_event event, void *unused)
{
	UNUSED_PARAMETER(unused);

	if (event == OBS_FRONTEND_EVENT_STREAMING_STARTED)
		irlsafety_hybrid_delay_on_stream_started();
	else if (event == OBS_FRONTEND_EVENT_STREAMING_STOPPED)
		irlsafety_hybrid_delay_on_stream_stopped();
}
#endif

#ifndef _WIN32
#define IRLSAFETY_HAS_OCR_PROBE 0
#define IRLSAFETY_HAS_ONNX_PROBE 0
#elif defined(IRLSAFETY_TEST_BUILD)
#define IRLSAFETY_HAS_OCR_PROBE 0
#define IRLSAFETY_HAS_ONNX_PROBE 0
#else
#include "ocr/ocr_backend.h"
#define IRLSAFETY_HAS_OCR_PROBE 1
#if defined(IRLSAFETY_ENABLE_ONNX)
#include "detection/yolo_onnx.h"
#define IRLSAFETY_HAS_ONNX_PROBE 1
#else
#define IRLSAFETY_HAS_ONNX_PROBE 0
#endif
#endif

OBS_DECLARE_MODULE()
#ifndef IRLSAFETY_TEST_BUILD
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")
#endif

bool obs_module_load(void)
{
	obs_register_source(&irlsafety_pii_filter);
	obs_log(LOG_INFO, "IRLSAFETY+ loaded (version %s)", PLUGIN_VERSION);
#if IRLSAFETY_HAS_OCR_PROBE
	if (ocr_backend_available())
		obs_log(LOG_INFO, "IRLSAFETY+: Windows OCR is ready");
	else
		obs_log(LOG_WARNING,
			"IRLSAFETY+: Windows OCR unavailable — enable a Windows text OCR language in Settings");
#endif
#if IRLSAFETY_HAS_ONNX_PROBE
	obs_log(LOG_INFO,
		"IRLSAFETY+: ONNX object detection ready — enable License Plates / Street Signs and place irlsafety-detect.onnx in plugin models folder");
#endif
#if defined(IRLSAFETY_HAS_CONTROL_DOCK)
	irlsafety_control_dock_register();
	obs_log(LOG_INFO, "IRLSAFETY+: Control dock ready — open Docks → IRLSAFETY+ Control");
#endif
#if defined(IRLSAFETY_HAS_FRONTEND_API) && !defined(IRLSAFETY_TEST_BUILD)
	obs_frontend_add_event_callback(irlsafety_frontend_event, NULL);
	obs_log(LOG_INFO, "IRLSAFETY+: Hybrid stream delay ready (default 0.5s global + 0.5s auto when protecting)");
#endif
	return true;
}

void obs_module_unload(void)
{
#if defined(IRLSAFETY_HAS_FRONTEND_API) && !defined(IRLSAFETY_TEST_BUILD)
	obs_frontend_remove_event_callback(irlsafety_frontend_event, NULL);
	irlsafety_hybrid_delay_on_stream_stopped();
#endif
#if defined(IRLSAFETY_HAS_CONTROL_DOCK)
	irlsafety_control_dock_unregister();
#endif
#if IRLSAFETY_HAS_OCR_PROBE
	ocr_backend_shutdown();
#endif
	obs_log(LOG_INFO, "IRLSAFETY+ unloaded");
}