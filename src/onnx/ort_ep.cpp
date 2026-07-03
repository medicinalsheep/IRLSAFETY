/*
 * IRLSAFETY+ — ONNX Runtime execution provider selection (P7).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "ort_ep.h"

#include <cstring>
#include <string>
#include <unordered_map>

void irlsafety_ort_default_session_opts(irlsafety_ort_session_opts *out)
{
	if (!out)
		return;

	out->intra_op_threads = 2;
	out->inter_op_threads = 1;
	out->ep_mode = IRLSAFETY_ORT_EP_AUTO;
	out->prefer_gpu = true;
}

static void write_ep_name(char *ep_name_out, size_t ep_name_size, const char *name)
{
	if (!ep_name_out || ep_name_size == 0)
		return;

	strncpy(ep_name_out, name ? name : "CPU", ep_name_size - 1);
	ep_name_out[ep_name_size - 1] = '\0';
}

static bool try_append_provider(Ort::SessionOptions &options, const char *provider_name,
				const std::unordered_map<std::string, std::string> &provider_options)
{
	try {
		options.AppendExecutionProvider(provider_name, provider_options);
		return true;
	} catch (...) {
		return false;
	}
}

void irlsafety_ort_apply_session_opts(Ort::SessionOptions &options, const irlsafety_ort_session_opts *cfg,
				     char *ep_name_out, size_t ep_name_size)
{
	irlsafety_ort_session_opts defaults;
	const irlsafety_ort_session_opts *use = cfg;

	if (!use) {
		irlsafety_ort_default_session_opts(&defaults);
		use = &defaults;
	}

	options.SetIntraOpNumThreads(use->intra_op_threads > 0 ? use->intra_op_threads : 2);
	options.SetInterOpNumThreads(use->inter_op_threads > 0 ? use->inter_op_threads : 1);
	options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

	if (use->ep_mode == IRLSAFETY_ORT_EP_CPU) {
		write_ep_name(ep_name_out, ep_name_size, "CPU");
		return;
	}

	const char *selected = "CPU";

#if defined(__ANDROID__)
	if (use->ep_mode == IRLSAFETY_ORT_EP_XNNPACK) {
		if (try_append_provider(options, "XNNPACK", {}))
			selected = "XNNPACK";
	} else if (use->ep_mode == IRLSAFETY_ORT_EP_NNAPI || use->prefer_gpu || use->ep_mode == IRLSAFETY_ORT_EP_AUTO) {
		if (try_append_provider(options, "NNAPI", {}))
			selected = "NNAPI";
		else if (try_append_provider(options, "XNNPACK", {}))
			selected = "XNNPACK";
	}
#elif defined(_WIN32)
	if (use->ep_mode == IRLSAFETY_ORT_EP_DML || use->prefer_gpu || use->ep_mode == IRLSAFETY_ORT_EP_AUTO) {
		std::unordered_map<std::string, std::string> dml_opts;
		dml_opts["device_id"] = "0";
		if (try_append_provider(options, "DmlExecutionProvider", dml_opts))
			selected = "DirectML";
	}
#else
	(void)selected;
	if (use->ep_mode == IRLSAFETY_ORT_EP_XNNPACK && try_append_provider(options, "XNNPACK", {}))
		selected = "XNNPACK";
#endif

	write_ep_name(ep_name_out, ep_name_size, selected);
}