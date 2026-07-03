/*
 * IRLSAFETY+ — per-filter censor activity ring log.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IRLSAFETY_CENSOR_LOG_CAPACITY 48

typedef enum irlsafety_censor_kind {
	IRLSAFETY_CENSOR_DETECT = 0,
	IRLSAFETY_CENSOR_OCR = 1,
	IRLSAFETY_CENSOR_SECURE = 2,
} irlsafety_censor_kind;

typedef struct irlsafety_censor_log_entry {
	uint64_t frame_index;
	uint64_t time_ms;
	irlsafety_censor_kind kind;
	uint32_t region_count;
	char detail[96];
} irlsafety_censor_log_entry;

typedef struct irlsafety_censor_log {
	irlsafety_censor_log_entry entries[IRLSAFETY_CENSOR_LOG_CAPACITY];
	size_t head;
	size_t count;
} irlsafety_censor_log;

void irlsafety_censor_log_init(irlsafety_censor_log *log);
void irlsafety_censor_log_clear(irlsafety_censor_log *log);
void irlsafety_censor_log_push(irlsafety_censor_log *log, uint64_t frame_index, irlsafety_censor_kind kind,
			       uint32_t region_count, const char *detail);
size_t irlsafety_censor_log_copy_recent(const irlsafety_censor_log *log, irlsafety_censor_log_entry *out,
					size_t max_entries);

#ifdef __cplusplus
}
#endif