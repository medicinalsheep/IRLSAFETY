/*
 * IRLSAFETY+ — per-filter censor activity ring log.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "censor_log.h"

#include "irlsafety_runtime.h"

#include <string.h>

void irlsafety_censor_log_init(irlsafety_censor_log *log)
{
	if (!log)
		return;

	memset(log, 0, sizeof(*log));
}

void irlsafety_censor_log_clear(irlsafety_censor_log *log)
{
	if (!log)
		return;

	log->head = 0;
	log->count = 0;
}

void irlsafety_censor_log_push(irlsafety_censor_log *log, uint64_t frame_index, irlsafety_censor_kind kind,
			       uint32_t region_count, const char *detail)
{
	irlsafety_censor_log_entry *entry;

	if (!log)
		return;

	entry = &log->entries[log->head];
	entry->frame_index = frame_index;
	entry->time_ms = irlsafety_monotonic_ms();
	entry->kind = kind;
	entry->region_count = region_count;

	if (detail && detail[0] != '\0') {
		strncpy(entry->detail, detail, sizeof(entry->detail) - 1);
		entry->detail[sizeof(entry->detail) - 1] = '\0';
	} else {
		entry->detail[0] = '\0';
	}

	log->head = (log->head + 1) % IRLSAFETY_CENSOR_LOG_CAPACITY;
	if (log->count < IRLSAFETY_CENSOR_LOG_CAPACITY)
		log->count++;
}

size_t irlsafety_censor_log_copy_recent(const irlsafety_censor_log *log, irlsafety_censor_log_entry *out,
					size_t max_entries)
{
	size_t copied = 0;
	size_t i;

	if (!log || !out || max_entries == 0)
		return 0;

	for (i = 0; i < log->count && copied < max_entries; i++) {
		size_t idx;

		if (log->count < IRLSAFETY_CENSOR_LOG_CAPACITY)
			idx = log->count - 1 - i;
		else
			idx = (log->head + IRLSAFETY_CENSOR_LOG_CAPACITY - 1 - i) % IRLSAFETY_CENSOR_LOG_CAPACITY;

		out[copied++] = log->entries[idx];
	}

	return copied;
}