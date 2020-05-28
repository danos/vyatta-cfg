/* Copyright (c) 2019-2020, AT&T Intellectual Property Inc. All rights reserved. */
/*
   Copyright (c) 2013-2016 by Brocade Communications Systems, Inc.
   All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
*/

#include <errno.h>
#include <argz.h>
#include <vyatta-util/vector.h>

#include "connect.h"
#include "error.h"
#include "transaction.h"
#include "internal.h"
#include "rpc.h"

static void argz_pop(char **argz, size_t *size)
{
	char *next = NULL;
	char *prev = NULL;

	while ((next = argz_next(*argz, *size, next)))
		prev = next;
	if (prev)
		argz_delete(argz, size, prev);
}

// split rename or copy path at "to" and have from and to be full paths
static int split_path(char **from, size_t *from_len, char **to, size_t *to_len)
{
	int i;
	char *new_to = NULL;
	size_t new_to_len = 0;
	char *next;
	size_t count = argz_count(*from, *from_len);

	// Don't copy the element before the "to"
	for (next = NULL, i = 0; i < (count - 3); ++i) {
		next = argz_next(*from, *from_len, next);
		if (!next)
			goto error;
		if (argz_add(&new_to, &new_to_len, next))
			goto error;
	}

	// i = count - 3 from previous for loop
	for ( ; i  < count; ++i) {
		next = argz_next(*from, *from_len, next); // skip "qualifier"
		if (!next)
			goto error;  // did not find "qualifier"
	}
	if (argz_add(&new_to, &new_to_len, next))
		goto error;

	// delete last 2 elements of from
	argz_pop(from, from_len);
	argz_pop(from, from_len);
	*to = new_to;
	*to_len = new_to_len;
	return 0;
error:
	free(new_to);
	return -1;
}

static int make_paths(const char *path, char **from_path, char **to_path)
{
	char *from;
	size_t from_len;
	char *to = NULL;
	size_t to_len;

	if (argz_create_sep(path, '/', &from, &from_len))
		return -1;
	if (split_path(&from, &from_len, &to, &to_len))
		goto error;
	if (argz_insert(&from, &from_len, from, ""))
		goto error;
	if (argz_insert(&to, &to_len, to, ""))
		goto error;
	argz_stringify(from, from_len, '/');
	*from_path = from;
	argz_stringify(to, to_len, '/');
	*to_path = to;
	return 0;
error:
	free(from);
	free(to);
	return -1;
}


char *configd_show(struct configd_conn *conn, int db, const char *cpath,
			   const char *cfg1, const char *cfg2, int flags,
			   struct configd_error *error)
{
	char *result;
	struct request req = { .fn = "Show" };
	if (flags & SHOWF_DEFAULTS) {
		req.fn = "ShowDefaults";
	}
	char *id = "";
	int hide_secrets = flags & SHOWF_HIDE_SECRETS;

	if (!cfg1 || !cfg2) {
		errno = EFAULT;
		return NULL;
	}

	if (!cpath)
		cpath = "";

	// TODO: this will be changing to client side diff using tree_get
	//req.args = json_pack("[sisssi]", conn->session_id, db, cpath, cfg1, cfg2, flags);
	if (conn->session_id) {
		id = conn->session_id;
	}
	req.args = json_pack("[issb]", db, id, cpath, hide_secrets);
	if (!req.args)
		return NULL;

	error_init(error, __func__);
	result = get_str(conn, &req, error);
	return result;
}

char *configd_set(struct configd_conn *conn, const char *cpath, struct configd_error *error)
{
	char *result;
	struct request req = { .fn = "Set" };

	req.args = json_pack("[ss]", conn->session_id, cpath);
	if (!req.args)
		return NULL;

	error_init(error, __func__);
	result = get_str(conn, &req, error);
	return result;
}

char *configd_validate_path(struct configd_conn *conn, const char *cpath, struct configd_error *error)
{
	char *result;
	struct request req = { .fn = "ValidatePath" };

	req.args = json_pack("[ss]", conn->session_id, cpath);
	if (!req.args)
		return NULL;

	error_init(error, __func__);
	result = get_str(conn, &req, error);
	return result;
}

char *configd_delete(struct configd_conn *conn, const char *cpath, struct configd_error *error)
{
	int result;
	struct request req = { .fn = "Delete" };

	req.args = json_pack("[ss]", conn->session_id, cpath);
	if (!req.args)
		return NULL;

	error_init(error, __func__);
	result = get_int(conn, &req, error);
	return (result == 1) ? strdup("") : NULL;
}

char *configd_rename(struct configd_conn *conn, const char *cpath, struct configd_error *error)
{
	char *result = NULL;
	struct request req = { .fn = "Rename" };
	char *from;
	char *to;

	if (make_paths(cpath, &from, &to))
		return NULL;

	req.args = json_pack("[sss]", conn->session_id, from, to);
	if (!req.args)
		goto error;

	error_init(error, __func__);
	result = get_str(conn, &req, error);
error:
	free(from);
	free(to);
	return result;
}

char *configd_copy(struct configd_conn *conn, const char *cpath, struct configd_error *error)
{
	char *result = NULL;
	struct request req = { .fn = "Copy" };
	char *from;
	char *to;

	if (make_paths(cpath, &from, &to))
		return NULL;

	req.args = json_pack("[sss]", conn->session_id, from, to);
	if (!req.args)
		goto error;

	error_init(error, __func__);
	result = get_str(conn, &req, error);
error:
	free(from);
	free(to);
	return result;
}

char *configd_comment(struct configd_conn *conn, const char *cpath, struct configd_error *error)
{
	int result;
	struct request req = { .fn = "Comment" };

	req.args = json_pack("[ss]", conn->session_id, cpath);
	if (!req.args)
		return NULL;

	error_init(error, __func__);
	result = get_int(conn, &req, error);
	return (result == 1) ? strdup("") : NULL;
}

char *configd_commit(struct configd_conn *conn, const char *comment, struct configd_error *error)
{
	char *result;
	struct request req = { .fn = "Commit" };

	req.args = json_pack("[ssb]", conn->session_id, comment, 0);
	if (!req.args)
		return NULL;

	error_init(error, __func__);
	result = get_str(conn, &req, error);
	configd_error_format_for_commit_or_val(error, "Commit");
	return result;
}

char *configd_confirmed_commit(struct configd_conn *conn,
			       const char *comment,
			       int confirmed,
			       const char *timeout, 
			       const char *persist, 
			       const char *persistid, 
			       struct configd_error *error)
{
	char *result;
	struct request req = { .fn = "ConfirmedCommit" };

	req.args = json_pack("[ssbsssb]", conn->session_id, comment, confirmed, timeout, persist, persistid, 0);
	if (!req.args)
		return NULL;

	error_init(error, __func__);
	result = get_str(conn, &req, error);
	configd_error_format_for_commit_or_val(error, "ConfirmedCommit");
	return result;
}

char *configd_cancel_commit(struct configd_conn *conn, const char *comment, const char *persistid, struct configd_error *error)
{
	char *result;
	struct request req = { .fn = "CancelCommit" };

	req.args = json_pack("[sssbb]", conn->session_id, comment, persistid, 0, 0);
	if (!req.args) {
		return NULL;
	}

	error_init(error, __func__);
	result = get_str(conn, &req, error);
	return result;
}

char *configd_discard(struct configd_conn *conn, struct configd_error *error)
{
	int result;
	struct request req = { .fn = "Discard" };

	req.args = json_pack("[s]", conn->session_id);
	if (!req.args)
		return NULL;

	error_init(error, __func__);
	result = get_int(conn, &req, error);
	return (result == 1) ? strdup("") : NULL;
}

char *configd_save(struct configd_conn *conn, const char *args, struct configd_error *error)
{
	struct request req = { .fn = "Save" };
	int result;

	/*
	 * The Save RPC only takes one argument which is already hardcoded to
	 * /config/config.boot therefore we only parse args in order to tell
	 * the user they shouldn't give any!
	 */
	if (args != NULL) {
		char *argz = NULL;
		size_t len = 0;

		argz_create_sep(args, ' ', &argz, &len);
		free(argz);
		if (len > 0) {
			error_init(error, __func__);
			error_setf(error, "Expected no arguments got %d", len);
			return NULL;
		}
	}

	req.args = json_pack("[s]", "/config/config.boot");
	if (!req.args)
		return NULL;

	error_init(error, __func__);
	result = get_int(conn, &req, error);
	return (result == 1) ? strdup("") : NULL;
}

int configd_load_internal(
	struct configd_conn *conn,
	const char *file,
	struct configd_error *error,
	const char *fnName)
{
	int result;
	struct request req = { .fn = fnName };

	req.args = json_pack("[ss]", conn->session_id, file);
	if (!req.args)
		return -1;

	error_init(error, __func__);
	result = get_int(conn, &req, error);
	return result;
}

int configd_load(
	struct configd_conn *conn,
	const char *file,
	struct configd_error *error)
{
	return configd_load_internal(conn, file, error, "Load");
}

int configd_load_report_warnings(
	struct configd_conn *conn,
	const char *file,
	struct configd_error *error)
{
	return configd_load_internal(conn, file, error, "LoadReportWarnings");
}

int configd_merge(struct configd_conn *conn, const char *file, struct configd_error *error)
{
	return configd_load_internal(conn, file, error, "MergeReportWarnings");
}

char *configd_validate(struct configd_conn *conn, struct configd_error *error)
{
	char *result;
	struct request req = { .fn = "Validate" };

	req.args = json_pack("[s]", conn->session_id);
	if (!req.args)
		return NULL;

	error_init(error, __func__);
	result = get_str(conn, &req, error);
	configd_error_format_for_commit_or_val(error, "Validate");
	return result;
}

char *configd_edit_config_xml(struct configd_conn *conn,
			      const char *config_target,
			      const char *default_operation,
			      const char *test_option,
			      const char *error_option,
			      const char *config,
			      struct configd_error *error)
{
	char *result;
	struct request req = { .fn = "EditConfigXML" };

	req.args = json_pack("[ssssss]", conn->session_id,
		config_target, default_operation, test_option,
		error_option, config);
	if (!req.args)
		return NULL;

	error_init(error, __func__);
	result = get_str(conn, &req, error);
	return result;
}
