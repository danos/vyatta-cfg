/*
 * Copyright (c) 2019-2020, AT&T Intellectual Property. All rights reserved.
 *
   Copyright (c) 2013-2014 by Brocade Communications Systems, Inc.
   All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
*/

#ifndef CONFIGD_ERROR_H_
#define CONFIGD_ERROR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <vyatta-util/map.h>

/**
 * configd_mgmt_error stores a configd mgmt_error which contains the fields
 * of an <rpc-error> as outlined in RFC 6241 (NETCONF) section 4.3.  Content
 * of these fields is covered in RFC 6241 Appendix A and RFC 6020 Section 13.
 */
struct configd_mgmt_error
{
	char *tag;        // JSON error-tag
	char *type;	  // JSON error-type; RFC 6241 Section 4.3
	char *severity;   // JSON error-severity; RFC 6241 Section 4.3
	// Following fields are optional and may not be encoded.
	char *app_tag;	  // JSON error-app-tag
	char *path;	  // JSON error-path
	char *message;	  // JSON error-message
	struct map *info; // JSON error-info
};

/**
 * configd_mgmt_error_app_tag returns error-app-tag field if set, or null.
 */
const char *configd_mgmt_error_app_tag(const struct configd_mgmt_error *me);

/**
 * configd_mgmt_error_message returns error-message field if set, or null.
 */
const char *configd_mgmt_error_message(const struct configd_mgmt_error *me);

/**
 * configd_mgmt_error_path returns error-path field if set, or null.
 */
const char *configd_mgmt_error_path(const struct configd_mgmt_error *me);

/**
 * configd_mgmt_error_tag returns error-tag field if set, or null.
 */
const char *configd_mgmt_error_tag(const struct configd_mgmt_error *me);

/**
 * configd_mgmt_error_type returns error-type field if set, or null.
 */
const char *configd_mgmt_error_type(const struct configd_mgmt_error *me);

/**
 * configd_mgmt_error_severity returns error-severity field if set, or null.
 */
const char *configd_mgmt_error_severity(const struct configd_mgmt_error *me);

/**
 * configd_mgmt_error_info returns map of error-info fields if set, or null.
 */
const struct map *configd_mgmt_error_info(const struct configd_mgmt_error *me);

/**
 * configd_mgmt_error_free frees a single configd_mgmt_error object.
 */
void configd_mgmt_error_free(struct configd_mgmt_error *me);

/**
 * configd_mgmt_error_set_from_maps sets the fields of a configd_mgmt_error
 * from the given fields and info maps extracted from the incoming JSON.
 */
int configd_mgmt_error_set_from_maps(
	struct configd_mgmt_error *me,
	struct map *fields,
	struct map **info);

/**
 * configd_mgmt_err_list stores a list of config_mgmt_errors.
 */
struct configd_mgmt_err_list
{
	int num_entries;
	struct configd_mgmt_error **me_list;
};

/**
 * The configd_error struct consists of the error message text and the function
 * it was generated in (source).
 * Additionally it may include some / all elements of 1 or more configd
 * MgmtError objects where these are sent in place of a basic error (string).
 * In this case, <is_mgmt_error> is set to true (!0)
 */
struct configd_error
{
	const char *source;
	char *text;
	int is_mgmt_error;
	struct configd_mgmt_err_list mgmt_errs;
};

/**
 * configd_error_is_mgmt_error returns non-zero value if error is a mgmt_error
 * and 0 if it is a basic (non-mgmt) error.
 */
int configd_error_is_mgmt_error(struct configd_error *ce);

/**
 * configd_error_num_mgmt_errors returns the number of mgmt_errors, if any,
 * embedded in the configd_error.
 */
int configd_error_num_mgmt_errors(struct configd_error *ce);

/**
 * configd_error_mgmt_error_list returns the list of mgmt_errors embedded in
 * a configd_error, or null if none.
 */
struct configd_mgmt_error **configd_error_mgmt_error_list(
	struct configd_error *ce);

/**
 * configd_error_format_for_commit_or_val formats the 'text' message in the
 * configd_error to a standard format for commit and validation messages that
 * matches what would be seen on the CLI.
 */
void configd_error_format_for_commit_or_val(
	struct configd_error *ce,
	const char *operation);

/**
 * configd_error_free frees the error struct and should be used when the error
 * message is no longer needed.
*/
void configd_error_free(struct configd_error *error);

#ifdef __cplusplus
}
#endif

#endif
