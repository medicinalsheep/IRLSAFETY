/*
 * IRLSAFETY+ — Android JNI bridge to libirlsafety (P10–P12).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "yuv_convert.h"

#include <android/log.h>
#include <jni.h>

#include <cstdio>
#include <cstring>
#include <vector>

#include "irlsafety.h"
#include "irlsafety_control.h"
#include "irlsafety_settings.h"

#define IRL_LOG_TAG "IRLSAFETY+"

struct AndroidPipeline {
	irlsafety_pipeline *core = nullptr;
	irlsafety_filter_settings settings {};
	uint64_t frame_index = 0;
	std::vector<uint8_t> bgra;
	uint32_t last_overlay_count = 0;
};

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

static AndroidPipeline *pipeline_from_handle(jlong handle)
{
	if (handle == 0)
		return nullptr;
	return reinterpret_cast<AndroidPipeline *>(static_cast<uintptr_t>(handle));
}

extern "C" JNIEXPORT jstring JNICALL Java_com_irlsafety_plus_IRLSafetyNative_nativeGetApiVersion(JNIEnv *env, jclass)
{
	ensure_android_logging();

	char buf[96];
	snprintf(buf, sizeof(buf), "libirlsafety API v%d · Android P12", IRLSAFETY_API_VERSION);
	return env->NewStringUTF(buf);
}

static void copy_jstring(JNIEnv *env, jstring src, char *dest, size_t dest_size)
{
	if (!dest || dest_size == 0)
		return;

	dest[0] = '\0';
	if (!src)
		return;

	const char *utf = env->GetStringUTFChars(src, nullptr);
	if (!utf)
		return;

	strncpy(dest, utf, dest_size - 1);
	dest[dest_size - 1] = '\0';
	env->ReleaseStringUTFChars(src, utf);
}

extern "C" JNIEXPORT jlong JNICALL Java_com_irlsafety_plus_IRLSafetyNative_nativeCreatePipeline(JNIEnv *env, jclass,
											      jstring model_path)
{
	ensure_android_logging();

	auto *wrap = new AndroidPipeline();
	wrap->core = irlsafety_pipeline_create();
	irlsafety_settings_apply_defaults(&wrap->settings);
	/* Android MVP: detection-first, no screen OCR. */
	wrap->settings.cat_screen_text = false;
	wrap->settings.cat_custom_pii = false;
	wrap->settings.cat_sensitive_patterns = false;
	wrap->settings.frame_skip = 6;
	wrap->settings.prefer_gpu = true;

	copy_jstring(env, model_path, wrap->settings.model_path, sizeof(wrap->settings.model_path));
	if (wrap->settings.model_path[0] != '\0') {
		irlsafety_pipeline_update_settings(wrap->core, &wrap->settings);
		__android_log_print(ANDROID_LOG_INFO, IRL_LOG_TAG, "Detection model path: %s", wrap->settings.model_path);
	} else {
		__android_log_print(ANDROID_LOG_WARN, IRL_LOG_TAG, "No detection model path — detector will stay unloaded");
	}

	return static_cast<jlong>(reinterpret_cast<uintptr_t>(wrap));
}

extern "C" JNIEXPORT void JNICALL Java_com_irlsafety_plus_IRLSafetyNative_nativeDestroyPipeline(JNIEnv *, jclass,
											      jlong handle)
{
	auto *wrap = pipeline_from_handle(handle);
	if (!wrap)
		return;

	if (wrap->core) {
		irlsafety_pipeline_shutdown(wrap->core);
		irlsafety_pipeline_destroy(wrap->core);
	}
	delete wrap;
}

extern "C" JNIEXPORT jstring JNICALL Java_com_irlsafety_plus_IRLSafetyNative_nativePipelineStatus(JNIEnv *env, jclass,
												       jlong handle)
{
	irlsafety_runtime_status status;
	char buf[320];

	ensure_android_logging();

	auto *wrap = pipeline_from_handle(handle);
	if (!wrap || !wrap->core)
		return env->NewStringUTF("Pipeline not created");

	irlsafety_pipeline_update_settings(wrap->core, &wrap->settings);
	irlsafety_pipeline_get_runtime_status(wrap->core, &status, wrap->frame_index);

	snprintf(buf, sizeof(buf),
		 "frames=%llu · overlays=%u · detector=%s · EP=%s · skip=%u",
		 static_cast<unsigned long long>(wrap->frame_index), wrap->last_overlay_count,
		 status.detector_ready ? "ready" : "stub",
		 status.detector_ep[0] ? status.detector_ep : "CPU", status.frame_skip);

	return env->NewStringUTF(buf);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_irlsafety_plus_IRLSafetyNative_nativeProcessCameraFrame(JNIEnv *env, jclass, jlong handle, jint width,
								       jint height, jbyteArray y_plane, jint y_row_stride,
								       jint y_pixel_stride, jbyteArray u_plane, jint u_row_stride,
								       jint u_pixel_stride, jbyteArray v_plane, jint v_row_stride,
								       jint v_pixel_stride)
{
	auto *wrap = pipeline_from_handle(handle);
	if (!wrap || !wrap->core || width <= 0 || height <= 0)
		return 0;

	const jsize y_len = env->GetArrayLength(y_plane);
	const jsize u_len = env->GetArrayLength(u_plane);
	const jsize v_len = env->GetArrayLength(v_plane);
	if (y_len <= 0 || u_len <= 0 || v_len <= 0)
		return 0;

	jbyte *y_bytes = env->GetByteArrayElements(y_plane, nullptr);
	jbyte *u_bytes = env->GetByteArrayElements(u_plane, nullptr);
	jbyte *v_bytes = env->GetByteArrayElements(v_plane, nullptr);
	if (!y_bytes || !u_bytes || !v_bytes) {
		if (y_bytes)
			env->ReleaseByteArrayElements(y_plane, y_bytes, JNI_ABORT);
		if (u_bytes)
			env->ReleaseByteArrayElements(u_plane, u_bytes, JNI_ABORT);
		if (v_bytes)
			env->ReleaseByteArrayElements(v_plane, v_bytes, JNI_ABORT);
		return wrap->last_overlay_count;
	}

	irlsafety_android_yuv420888_to_bgra(reinterpret_cast<const uint8_t *>(y_bytes), y_row_stride, y_pixel_stride,
					    reinterpret_cast<const uint8_t *>(u_bytes), u_row_stride, u_pixel_stride,
					    reinterpret_cast<const uint8_t *>(v_bytes), v_row_stride, v_pixel_stride,
					    width, height, wrap->bgra);

	env->ReleaseByteArrayElements(y_plane, y_bytes, JNI_ABORT);
	env->ReleaseByteArrayElements(u_plane, u_bytes, JNI_ABORT);
	env->ReleaseByteArrayElements(v_plane, v_bytes, JNI_ABORT);

	irlsafety_frame_view frame {};
	frame.planes[0] = wrap->bgra.data();
	frame.linesize[0] = static_cast<uint32_t>(width) * 4u;
	frame.width = static_cast<uint32_t>(width);
	frame.height = static_cast<uint32_t>(height);
	frame.format = IRLSAFETY_FORMAT_BGRA;
	frame.plane_count = 1;

	irlsafety_pipeline_update_settings(wrap->core, &wrap->settings);
	irlsafety_pipeline_detect_frame(wrap->core, &frame, &wrap->settings, wrap->frame_index++, frame.width,
					frame.height, false);

	irlsafety_region_list regions {};
	irlsafety_pipeline_get_overlays(wrap->core, frame.width, frame.height, &wrap->settings, &regions);
	wrap->last_overlay_count = static_cast<uint32_t>(regions.count);
	return static_cast<jint>(wrap->last_overlay_count);
}

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM *, void *)
{
	ensure_android_logging();
	return JNI_VERSION_1_6;
}