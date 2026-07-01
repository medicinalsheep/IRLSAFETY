/*
 * IRLSAFETY+ — custom PII keyword list (inline text + optional file).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IRLSAFETY_CUSTOM_PII_MAX_ENTRIES 128
#define IRLSAFETY_CUSTOM_PII_ENTRY_LEN 256

typedef struct irlsafety_custom_pii_list {
	char entries[IRLSAFETY_CUSTOM_PII_MAX_ENTRIES][IRLSAFETY_CUSTOM_PII_ENTRY_LEN];
	size_t count;
} irlsafety_custom_pii_list;

/* Parse inline multiline text and optional file (one entry per line, # comments allowed). */
int irlsafety_custom_pii_load(const char *inline_text, const char *file_path, irlsafety_custom_pii_list *out);

#ifdef __cplusplus
}
#endif