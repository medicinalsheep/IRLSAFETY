/*
 * IRLSAFETY+ — Android JNI bridge to libirlsafety (P10).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include <android/log.h>
#include <jni.h>

#include <cstdio>
#include <cstring>

#include "irlsafety.h"
#include "irlsafety_control.h"
#include "irlsafety_settings.h"

#define IRL_LOG_TAG "IRLSAFETY+"

static void android_log_bridge(int level, const char *message, void *)
{
	if (!message)
		return;

	int priority = ANDROID_LOG_INFO;
	if (level <= 100)
		priority = ANDROID_LOG_ERROR;
	else if (level <= 200)
		priority = ANDROID_LOG_WARN;
	else if (level >= 400)
		priority = ANDROID_LOG_DEBUG;

	__android_log_print(priority, IRL_LOG_TAG, "%s", message);
}

static void ensure_android_logging(void)
{
	static bool installed = false;
	if (!installed) {
		irlsafety_log_set_callback(android_log_bridge, nullptr);
		installed = true;
	}
}

extern "C" JNIEXPORT jstring JNICALL Java_com_irlsafety_plus_IRLSafetyNative_nativeGetApiVersion(JNIEnv *env, jclass)
{
	ensure_android_logging();

	char buf[96];
	snprintf(buf, sizeof(buf), "libirlsafety API v%d (Android P10)", IRLSAFETY_API_VERSION);
	return env->NewStringUTF(buf);
}

extern "C" JNIEXPORT jlong JNICALL Java_com_irlsafety_plus_IRLSafetyNative_nativeCreatePipeline(JNIEnv *, jclass)
{
	ensure_android_logging();
	return static_cast<jlong>(reinterpret_cast<uintptr_t>(irlsafety_pipeline_create()));
}

extern "C" JNIEXPORT void JNICALL Java_com_irlsafety_plus_IRLSafetyNative_nativeDestroyPipeline(JNIEnv *, jclass, jlong handle)
{
	if (handle == 0)
		return;

	auto *pipeline = reinterpret_cast<irlsafety_pipeline *>(static_cast<uintptr_t>(handle));
	irlsafety_pipeline_shutdown(pipeline);
	irlsafety_pipeline_destroy(pipeline);
}

extern "C" JNIEXPORT jstring JNICALL Java_com_irlsafety_plus_IRLSafetyNative_nativePipelineStatus(JNIEnv *env, jclass, jlong handle)
{
	irlsafety_filter_settings settings;
	irlsafety_runtime_status status;
	char buf[256];

	ensure_android_logging();
	irlsafety_settings_apply_defaults(&settings);

	if (handle == 0) {
		return env->NewStringUTF("Pipeline not created");
	}

	auto *pipeline = reinterpret_cast<irlsafety_pipeline *>(static_cast<uintptr_t>(handle));
	irlsafety_pipeline_update_settings(pipeline, &settings);
	irlsafety_pipeline_get_runtime_status(pipeline, &status, 0);

	snprintf(buf, sizeof(buf),
		 "detector=%s · OCR=%s · frame_skip=%u · onnx=%s",
		 status.detector_ready ? "ready" : "stub",
		 status.ocr_available ? "on" : "off",
		 status.frame_skip,
		 status.detector_message[0] ? status.detector_message : "idle");

	return env->NewStringUTF(buf);
}

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM *, void *)
{
	ensure_android_logging();
	return JNI_VERSION_1_6;
}