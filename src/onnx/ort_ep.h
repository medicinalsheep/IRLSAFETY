/*
 * IRLSAFETY+ — ONNX Runtime execution provider selection (P7).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 *
 * Centralizes CPU / DirectML / NNAPI / XNNPACK EP wiring for YOLO and child OCR.
 */

#pragma once

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
#include <onnxruntime_cxx_api.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum irlsafety_ort_ep_mode {
	IRLSAFETY_ORT_EP_AUTO = 0,
	IRLSAFETY_ORT_EP_CPU = 1,
	IRLSAFETY_ORT_EP_DML = 2,
	IRLSAFETY_ORT_EP_NNAPI = 3,
	IRLSAFETY_ORT_EP_XNNPACK = 4,
} irlsafety_ort_ep_mode;

typedef struct irlsafety_ort_session_opts {
	int intra_op_threads;
	int inter_op_threads;
	irlsafety_ort_ep_mode ep_mode;
	bool prefer_gpu;
} irlsafety_ort_session_opts;

void irlsafety_ort_default_session_opts(irlsafety_ort_session_opts *out);

#ifdef __cplusplus
void irlsafety_ort_apply_session_opts(Ort::SessionOptions &options, const irlsafety_ort_session_opts *cfg,
				      char *ep_name_out, size_t ep_name_size);
#endif

#ifdef __cplusplus
}
#endif