/*
 * IRLSAFETY+ — OBS plugin module entry.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include <obs-module.h>
#include <plugin-support.h>

#include "blur/overlay_image.h"
#include "pii-filter.h"
#include "irlsafety_obs_adapter.h"
#include "irlsafety_paths.h"
#include "irlsafety_shutdown.h"
#include "virtual_cam/virtual_cam.h"

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

	if (event == OBS_FRONTEND_EVENT_EXIT) {
		irlsafety_begin_shutdown();
#if defined(IRLSAFETY_HAS_CONTROL_DOCK)
		irlsafety_control_dock_unregister();
#endif
		irlsafety_obs_on_stream_stopped();
	} else if (event == OBS_FRONTEND_EVENT_STREAMING_STARTED)
		irlsafety_obs_on_stream_started();
	else if (event == OBS_FRONTEND_EVENT_STREAMING_STOPPED)
		irlsafety_obs_on_stream_stopped();
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
	irlsafety_obs_adapter_register();
	obs_register_source(&irlsafety_pii_filter);
	obs_log(LOG_INFO, "IRLSAFETY+ loaded (version %s)", PLUGIN_VERSION);
#if IRLSAFETY_HAS_OCR_PROBE
	{
#if defined(IRLSAFETY_OCR_BACKEND_CHILD)
		char det_model[1024];
		char rec_model[1024];

		irlsafety_resolve_model_path("models/irlsafety-ocr-det.onnx", NULL, det_model, sizeof(det_model));
		irlsafety_resolve_model_path("models/irlsafety-ocr-rec.onnx", NULL, rec_model, sizeof(rec_model));
		ocr_backend_configure_models(det_model[0] != '\0' ? det_model : NULL,
					     rec_model[0] != '\0' ? rec_model : NULL);
#else
		char ocr_model[1024];

		irlsafety_resolve_model_path("models/irlsafety-ocr.onnx", NULL, ocr_model, sizeof(ocr_model));
		ocr_backend_configure(ocr_model[0] != '\0' ? ocr_model : NULL);
#endif
		obs_log(LOG_INFO, "IRLSAFETY+: %s — %s", ocr_backend_name(), ocr_backend_status_message());
	}
#endif
	irlsafety_virtual_cam_refresh_status();
	if (!irlsafety_virtual_cam_supported())
		obs_log(LOG_INFO, "IRLSAFETY+: Virtual camera — %s", irlsafety_virtual_cam_status_message());
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
	obs_log(LOG_INFO, "IRLSAFETY+: Hybrid stream delay ready (default 1.5s global + 1.0s auto when protecting)");
#endif
	return true;
}

void obs_module_unload(void)
{
	irlsafety_begin_shutdown();
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
	irlsafety_overlay_release_cache();
	irlsafety_obs_adapter_unregister();
	obs_log(LOG_INFO, "IRLSAFETY+ unloaded");
}