/*
 * IRLSAFETY+ — custom PII keyword list (inline text + optional file).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "custom_pii.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static void trim_inplace(char *line)
{
	char *start = line;
	char *end;

	while (*start && isspace((unsigned char)*start))
		start++;

	if (start != line)
		memmove(line, start, strlen(start) + 1);

	end = line + strlen(line);
	while (end > line && isspace((unsigned char)*(end - 1)))
		end--;
	*end = '\0';
}

static int add_entry(irlsafety_custom_pii_list *out, const char *entry)
{
	if (!out || !entry || entry[0] == '\0')
		return 0;

	if (out->count >= IRLSAFETY_CUSTOM_PII_MAX_ENTRIES)
		return -1;

	strncpy(out->entries[out->count], entry, IRLSAFETY_CUSTOM_PII_ENTRY_LEN - 1);
	out->entries[out->count][IRLSAFETY_CUSTOM_PII_ENTRY_LEN - 1] = '\0';
	out->count++;
	return 0;
}

static int parse_text_buffer(const char *text, irlsafety_custom_pii_list *out)
{
	char buffer[IRLSAFETY_CUSTOM_PII_ENTRY_LEN];
	const char *cursor = text;
	const char *line_end;

	if (!text)
		return 0;

	while (*cursor) {
		line_end = strchr(cursor, '\n');
		if (!line_end)
			line_end = cursor + strlen(cursor);

		size_t len = (size_t)(line_end - cursor);
		if (len >= sizeof(buffer))
			len = sizeof(buffer) - 1;

		memcpy(buffer, cursor, len);
		buffer[len] = '\0';
		trim_inplace(buffer);

		if (buffer[0] != '\0' && buffer[0] != '#') {
			if (add_entry(out, buffer) != 0)
				return -1;
		}

		if (*line_end == '\0')
			break;
		cursor = line_end + 1;
	}

	return 0;
}

static int parse_file(const char *file_path, irlsafety_custom_pii_list *out)
{
	FILE *fp;
	char line[IRLSAFETY_CUSTOM_PII_ENTRY_LEN];

	if (!file_path || file_path[0] == '\0')
		return 0;

	fp = fopen(file_path, "r");
	if (!fp)
		return -1;

	while (fgets(line, sizeof(line), fp)) {
		trim_inplace(line);
		if (line[0] == '\0' || line[0] == '#')
			continue;
		if (add_entry(out, line) != 0) {
			fclose(fp);
			return -1;
		}
	}

	fclose(fp);
	return 0;
}

int irlsafety_custom_pii_load(const char *inline_text, const char *file_path, irlsafety_custom_pii_list *out)
{
	if (!out)
		return -1;

	memset(out, 0, sizeof(*out));

	if (parse_text_buffer(inline_text, out) != 0)
		return -1;

	return parse_file(file_path, out);
}