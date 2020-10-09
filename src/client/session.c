/*
 * Copyright (c) 2019-2020, AT&T Intellectual Property. All rights reserved.
 *
   Copyright (c) 2013-2014 by Brocade Communications Systems, Inc.
   All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
*/

#include <argz.h>
#include <string.h>
#include <vyatta-util/map.h>

#include "connect.h"
#include "error.h"
#include "node.h"
#include "session.h"
#include "template.h"
#include "transaction.h"
#include "internal.h"
#include "rpc.h"

#define C_ENV_EDIT_LEVEL "VYATTA_EDIT_LEVEL"

int configd_sess_exists(struct configd_conn *conn, struct configd_error *error)
{
	int result;
	struct request req = { .fn = "SessionExists" };

	req.args = json_pack("[s]", conn->session_id);
	if (!req.args)
		return -1;

	error_init(error, __func__);
	result = get_int(conn, &req, error);
	return result;
}

static int _configd_sess_setup(struct configd_conn *conn,
							   struct configd_error *error,
							   const char *fn)
{
	int result;
	struct request req = { .fn = fn };

	req.args = json_pack("[s]", conn->session_id);
	if (!req.args)
		return -1;

	error_init(error, __func__);
	result = get_int(conn, &req, error);
	return result != -1 ? !result : -1;
}

int configd_sess_setup(struct configd_conn *conn, struct configd_error *error)
{
	return _configd_sess_setup(conn, error, "SessionSetup");
}

int configd_sess_setup_shared(struct configd_conn *conn, struct configd_error *error)
{
	return _configd_sess_setup(conn, error, "SessionSetupShared");
}

int configd_sess_teardown(struct configd_conn *conn, struct configd_error *error)
{
	int result;
	struct request req = { .fn = "SessionTeardown" };

	req.args = json_pack("[s]", conn->session_id);
	if (!req.args)
		return -1;

	error_init(error, __func__);
	result = get_int(conn, &req, error);
	return result != -1 ? !result : -1;
}

int configd_sess_lock(struct configd_conn *conn, struct configd_error *error)
{
	int result;
	struct request req = { .fn = "SessionLock" };

	req.args = json_pack("[s]", conn->session_id);
	if (!req.args)
		return -1;

	error_init(error, __func__);
	result = get_int(conn, &req, error);
	return result;
}

int configd_sess_unlock(struct configd_conn *conn, struct configd_error *error)
{
	int result;
	struct request req = { .fn = "SessionUnlock" };

	req.args = json_pack("[s]", conn->session_id);
	if (!req.args)
		return -1;

	error_init(error, __func__);
	result = get_int(conn, &req, error);
	return result;
}

int configd_sess_locked(struct configd_conn *conn, struct configd_error *error)
{
	int result;
	struct request req = { .fn = "SessionLocked" };

	req.args = json_pack("[s]", conn->session_id);
	if (!req.args)
		return -1;

	error_init(error, __func__);
	result = get_int(conn, &req, error);
	return result;
}

int configd_sess_changed(struct configd_conn *conn, struct configd_error *error)
{
	int result;
	struct request req = { .fn = "SessionChanged" };

	req.args = json_pack("[s]", conn->session_id);
	if (!req.args)
		return -1;

	error_init(error, __func__);
	result = get_int(conn, &req, error);
	return result;
}

int configd_sess_saved(struct configd_conn *conn, struct configd_error *error)
{
	int result;
	struct request req = { .fn = "SessionSaved" };

	req.args = json_pack("[s]", conn->session_id);
	if (!req.args)
		return -1;

	error_init(error, __func__);
	result = get_int(conn, &req, error);
	return result;
}

int configd_sess_mark_saved(struct configd_conn *conn, struct configd_error *error)
{
	int result;
	struct request req = { .fn = "SessionMarkSaved" };

	req.args = json_pack("[s]", conn->session_id);
	if (!req.args)
		return -1;

	error_init(error, __func__);
	result = get_int(conn, &req, error);
	return result != -1 ? !result : -1;
}

int configd_sess_mark_unsaved(struct configd_conn *conn, struct configd_error *error)
{
	int result;
	struct request req = { .fn = "SessionMarkUnsaved" };

	req.args = json_pack("[s]", conn->session_id);
	if (!req.args)
		return -1;

	error_init(error, __func__);
	result = get_int(conn, &req, error);
	return result != -1 ? !result : -1;
}

char *configd_sess_get_env(struct configd_conn *conn, struct configd_error *error)
{
	static const char env_fmt[] = "declare -x " C_ENV_EDIT_LEVEL "=/; "
		"umask 002; { declare -x -r VYATTA_CONFIG_SID=%s; } >&/dev/null || true";
	char *env;

	if (asprintf(&env, env_fmt, conn->session_id) == -1)
		return NULL;
	return env;
}


static int is_tag_value(struct map *m)
{
	const char *value = map_get(m, "is_value");
	const char *tag = map_get(m, "tag");
	int is_value = value && (strcmp(value, "1") == 0);
	int is_tag = tag && (strcmp(tag, "1") == 0);
	return (is_tag && is_value);
}

static int is_typeless(struct map *m)
{
	const char *type = map_get(m, "type");
	return type == NULL;
}


char *configd_edit_get_env(struct configd_conn *conn, const char *cpath, struct configd_error *error)
{
	static const char env_fmt[] = "export " C_ENV_EDIT_LEVEL "='%s'; "
		"export PS1='[edit %s]\\n\\u@\\h# ';";
	char *result;
	char *prompt_path;
	size_t ppath_size;
	struct map *def = configd_tmpl_get(conn, cpath, error);
	if (!def)
		return NULL;

	error_init(error, __func__);
	/* "edit" is only allowed when path ends at a
	 *   (1) "tag value"
	 *   OR
	 *   (2) "typeless node"
	 */
	if (!is_tag_value(def) && !is_typeless(def)) {
		/* neither "tag value" nor "typeless node" */
		error_setf(error, "The \"edit\" command cannot be issued "
			   "at the specified level\n");
		return NULL;
	}
	if (configd_node_exists(conn, AUTO, cpath, error) != 1) {
		/* specified path does not exist.
		 * follow the original implementation and do a "set".
		 */
		result = configd_set(conn, cpath, error);
		if (!result)
			return NULL;
		free(result);
	}

	if (argz_create_sep(cpath, '/', &prompt_path, &ppath_size))
		return NULL;
	argz_stringify(prompt_path, ppath_size, ' ');
	if (asprintf(&result, env_fmt, cpath, prompt_path) == -1) {
		free(prompt_path);
		return NULL;
	}
	free(prompt_path);
	return result;
}

struct map *configd_get_help(struct configd_conn *conn, int from_schema, const char *cpath, struct configd_error *error)
{
	struct map *result;
	struct request req = { .fn = "GetHelp" };

	req.args = json_pack("[sbs]", conn->session_id, from_schema, cpath);
	if (!req.args)
		return NULL;

	error_init(error, __func__);
	result = get_map(conn, &req, error);
	return result;
}
