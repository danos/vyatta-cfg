/*
 * Copyright (c) 2020, AT&T Intellectual Property. All rights reserved.
 *
   Copyright (c) 2013-2014 by Brocade Communications Systems, Inc.
   All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
*/

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <vyatta-util/map.h>
#include <vyatta-util/vector.h>

#include "log.h"
#include "error.h"
#include "internal.h"

// These strings need to match the JSON tags in golang-github-danos-mgmterror
// MgmtError structure.
#define ERROR_APP_TAG "error-app-tag"
#define ERROR_INFO "error-info"
#define ERROR_MESSAGE "error-message"
#define ERROR_PATH "error-path"
#define ERROR_SEVERITY "error-severity"
#define ERROR_TAG "error-tag"
#define ERROR_TYPE "error-type"


void configd_mgmt_error_free(struct configd_mgmt_error *me)
{
	if (me->tag != NULL) {
		free(me->tag);
		me->tag = NULL;
	}

	if (me->type != NULL) {
		free(me->type);
		me->type = NULL;
	}
	if (me->severity != NULL) {
		free(me->severity);
		me->severity = NULL;
	}
	if (me->app_tag != NULL) {
		free(me->app_tag);
		me->app_tag = NULL;
	}
	if (me->path != NULL) {
		free(me->path);
		me->path = NULL;
	}
	if (me->message != NULL) {
		free(me->message);
		me->message = NULL;
	}

	if (me->info != NULL) {
		map_free(me->info);
		me->info = NULL;
	}
}

const char *configd_mgmt_error_app_tag(const struct configd_mgmt_error *me) {
	return me ? me->app_tag : NULL;
}

const char *configd_mgmt_error_message(const struct configd_mgmt_error *me) {
	return me ? me->message : NULL;
}

const char *configd_mgmt_error_path(const struct configd_mgmt_error *me) {
	return me ? me->path : NULL;
}

const char *configd_mgmt_error_severity(const struct configd_mgmt_error *me) {
	return me ? me->severity : NULL;
}

const char *configd_mgmt_error_tag(const struct configd_mgmt_error *me) {
	return me ? me->tag : NULL;
}

const char *configd_mgmt_error_type(const struct configd_mgmt_error *me) {
	return me ? me->type : NULL;
}

const struct map *configd_mgmt_error_info(const struct configd_mgmt_error *me) {
	return me ? me->info : NULL;
}

static int set_value(
	const char *value,
	char **field_to_set)
{
	if (value != NULL) {
		*field_to_set = local_strdup(value);
		if (!*field_to_set) {
			return -1;
		}
	}

	return 0;
}

// If we populate 'info', we take on ownership of the map passed in to us.
int configd_mgmt_error_set_from_maps(
	struct configd_mgmt_error *me,
	struct map *fields,
	struct map **info)
{
	int retval = 0;

	// info is optional.
	if (me == NULL || fields == NULL) {
		errno = EINVAL;
		return -1;
	}

	// Tag, type and severity are mandatory - for now we will assume that
	// they are provided and not throw an error if missing.
	retval |= set_value(map_get(fields, ERROR_TAG), &me->tag);
	retval |= set_value(map_get(fields, ERROR_TYPE), &me->type);
	retval |= set_value(map_get(fields, ERROR_SEVERITY), &me->severity);

	// Remaining fields are optional.
	retval |= set_value(map_get(fields, ERROR_APP_TAG), &me->app_tag);
	retval |= set_value(map_get(fields, ERROR_PATH), &me->path);
	retval |= set_value(map_get(fields, ERROR_MESSAGE), &me->message);

	if (*info != NULL)
	{
		me->info = *info;
		*info = NULL;
	}

	return retval;
}

// Return space-separated version of path, or NULL if no path set.
// Ignore any leading '/'.
static char *configd_mgmt_error_path_space_separated(
	struct configd_mgmt_error *me)
{
	if (me == NULL) {
		return NULL;
	}

	if ((me->path == NULL) || (me->path[0] == '\0')) {
		return NULL;
	}

	// Get copy of path to modify inline, ignoring leading '/' if present.
	int len = strlen(me->path) + 1;
	char *path = malloc(len);
	if (path == NULL) {
		return NULL;
	}
	snprintf(path, len, "%s", (me->path[0] != '/' ? me->path : &me->path[1]));

	int path_len = strlen(path);
	int i = 0;

	for (i = 0;i < path_len; i ++) {
		if (path[i] == '/') {
			if (i < (path_len - 1)) {
				path[i] = ' ';
			} else {
				path[i] = '\0'; // Don't want trailing space.
			}
		}
	}
	return path;
}

// Return formatted error (multiline, ending in newline) or NULL.
//
// Format is as follows:
//
// --- START ERROR ---
// [path space separated]
//
// err.Message
//
// [[path space separated]] failed.
// --- END ERROR ---
static char *configd_mgmt_error_format_for_commit_or_val(
	struct configd_mgmt_error *me) {

	const char *fmt_str = "[%s]\n\n%s\n\n[[%s]] failed.\n";
	char *output = NULL, *path = NULL;

	if (me == NULL) {
		return NULL;
	}

	path = configd_mgmt_error_path_space_separated(me);
	int len =
		strlen(fmt_str)
		+ (me->message ? strlen(me->message) : 0)
		+ (path ? (2 * strlen(path)) : 0)
		+ 1;
	output = malloc(len);
	if (output == NULL) {
		if (path != NULL) {
			free(path);
		}
		return NULL;
	}
	snprintf(output, len, fmt_str,
		path ? path : "",
		me->message ? me->message : "",
		path ? path : "");

	if (path) {
		free(path);
	}
	return output;
}

void error_init(struct configd_error *error, const char *source)
{
	if (error != NULL) {
		error->text = NULL;
		error->source = source;
		error->is_mgmt_error = 0;
		error->mgmt_errs.num_entries = 0;
		error->mgmt_errs.me_list = NULL;
	}
}

// Attempt to construct a 'cli-output-formatted' entry for the 'text' field
// of <ce> to overwrite the default.  This is intended to match the format that
// would be output on the CLI when a validation or commit failure occurs.
// This was originally done on the 'send' side of the configd socket but now
// that MgmtErrors are passed with all fields intact, we need to perform this
// formatting on the receive side.
//
// The more cumbersome malloc/sprintf pairing is used over asprintf so that
// CppUTest memory leak detection can be used when testing this code.
//
// Format is as follows:
//
// --- START ---
// --- START ERROR ---
// [path space separated]
//
// err.Message
//
// [[path space separated]] failed.
// --- END ERROR ---
// --- NEXT ERROR IF ANY STARTS, NO BLANK LINE
// --- END LAST ERROR ---
//
// <operation> failed!
// --- END ---
//
void configd_error_format_for_commit_or_val(
	struct configd_error *ce,
	const char *operation)
{
	if ((ce == NULL) || (!ce->is_mgmt_error)) {
		return;
	}

	char *cur_output = NULL, *new_output, *err_str;
	const char *fmt_str = "\n%s failed!\n";
	operation = operation ? operation : "Operation";
	int len = strlen(fmt_str) + strlen(operation) + 1;
	cur_output = malloc(len);
	if (cur_output == NULL) {
		return;
	}
	snprintf(cur_output, len, fmt_str, operation);
	new_output = cur_output;

	int i = 0;

	struct configd_mgmt_error **me_list = configd_error_mgmt_error_list(ce);
	if (me_list == NULL) {
		return;
	}
	int num_errors = configd_error_num_mgmt_errors(ce);

	for (i = 0; i < num_errors; i ++) {
		err_str = configd_mgmt_error_format_for_commit_or_val(me_list[i]);
		int new_len = strlen(cur_output) + strlen(err_str) + 1;
		new_output = malloc(new_len);
		if (new_output == NULL) {
			if (err_str != NULL) {
				free(err_str);
			}
			// Use what we have so far ...
			new_output = cur_output;
			cur_output = NULL;
			break;
		}
		snprintf(new_output, new_len, "%s%s", (err_str ? err_str : ""),
			cur_output);

		if (err_str != NULL) {
			free(err_str);
			err_str = NULL;
		}
		free(cur_output);
		cur_output = new_output;
	}

	if (ce->text != NULL) {
		free(ce->text);
	}
	ce->text = new_output;
}

void configd_error_free(struct configd_error *error)
{
	if (error != NULL) {
		free(error->text);
		error->text = NULL;
		error->source = NULL;

		if (error->mgmt_errs.me_list != NULL) {
			mgmt_err_list_free(&error->mgmt_errs);
		}
	}
}

int configd_error_is_mgmt_error(struct configd_error *error) {
	return error->is_mgmt_error;
}

int configd_error_num_mgmt_errors(struct configd_error *error) {
	return error->mgmt_errs.num_entries;
}

struct configd_mgmt_error **configd_error_mgmt_error_list(
	struct configd_error *ce) {
	return ce->mgmt_errs.me_list;
}

int error_setf(struct configd_error *error, const char *msg, ...)
{
	int result;
	va_list ap;

	va_start(ap, msg);
	result = error_vsetf(error, msg, ap);
	va_end(ap);
	return result;
}

int error_vsetf(struct configd_error *error, const char *msg, va_list ap)
{
	if (error == NULL || msg == NULL) {
		errno = EINVAL;
		return -1;
	}

	return local_vasprintf(&error->text, msg, ap);
}

void mgmt_err_list_free(struct configd_mgmt_err_list *mel)
{
	int i = 0;

	for (i = 0; i < mel->num_entries; i++) {
		configd_mgmt_error_free(mel->me_list[i]);
		free(mel->me_list[i]);
	}
	free(mel->me_list);
	mel->num_entries = 0;
	mel->me_list = NULL;
}

// Return malloced string containing the message from each mgmt_error in the
// list, one per line, with each entry including last terminated with newline.
static char *merge_messages(struct configd_mgmt_err_list *mgmt_errs)
{
	char *text = NULL;
	int bytes = 0;
	int i = 0;
	for (i = 0; i < mgmt_errs->num_entries; i++) {
		bytes += strlen(configd_mgmt_error_message(mgmt_errs->me_list[i]))
			+ 1; // +n/l
	}
	bytes += 1; // Allow for terminating null.

	text = malloc(bytes);
	if (text == NULL) {
		return NULL;
	}
	bzero(text, bytes);

	for (i = 0; i < mgmt_errs->num_entries; i++) {
		strncat(text, configd_mgmt_error_message(mgmt_errs->me_list[i]), bytes);
		strncat(text, "\n", bytes);
	}

	return text;
}


// Return 0 (success) / -1 (fail)
int error_set_from_mgmt_error_list(
	struct configd_error *ce,
	struct configd_mgmt_err_list *mgmt_errs,
	const char *function)
{
	if (ce == NULL) {
		return -1;
	}

	ce->mgmt_errs.num_entries = mgmt_errs->num_entries;
	ce->mgmt_errs.me_list = mgmt_errs->me_list;
	ce->is_mgmt_error = 1;

	// Rather than strdup etc, we transfer ownership.
	mgmt_errs->num_entries = 0;
	mgmt_errs->me_list = NULL;

	// To allow for clients that are unaware of / do not wish to handle
	// multiple errors, we set the 'text' field in the basic error to the
	// message field (if we have a single management error), or a concatenation
	// of all messages if we have more than one, one per line.
	// In all cases the message should end with a newline.

	if (ce->mgmt_errs.num_entries == 0) {
		return set_value("Unspecified error occurred (no data)\n", &ce->text);
	} else if (ce->mgmt_errs.num_entries == 1) {
		struct configd_mgmt_error *me = ce->mgmt_errs.me_list[0];
		if (me == NULL) {
			return set_value("Unspecified error (no content)\n", &ce->text);
		}
		return set_value(configd_mgmt_error_message(me), &ce->text);
	}

	char *messages = merge_messages(&ce->mgmt_errs);
	int retval = set_value(messages, &ce->text);
	free(messages);
	return retval;
}

// This allows CppUTest to track memory allocation, and thus to check for
// memory leaks.  Standard strdup doesn't have the null check on 's' but it
// doesn't hurt.
char *local_strdup(const char *s)
{
	if (s == NULL) {
		return NULL;
	}

	int len = strlen(s) + 1;

	char *retstr = malloc(len);
	if (retstr == NULL) {
		return NULL;
	}
	return strcpy(retstr, s);
}

int local_vasprintf (char **dest, const char *fmt_str, va_list args)
{
	int len = 0;
	*dest = 0;

	va_list ap_copy;
	va_copy(ap_copy, args);
	len = vsnprintf(NULL, 0, fmt_str, args);
	if (len == -1) {
		va_end(ap_copy);
		return -1;
	}

	*dest = malloc(len + 1);
	if (*dest == NULL) {
		va_end(ap_copy);
		return -1;
	}

	int retval = vsnprintf(*dest, len + 1, fmt_str, ap_copy);
	va_end(ap_copy);
	return retval;
}
