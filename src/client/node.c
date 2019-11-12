/*
 * Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
 *
   Copyright (c) 2013-2015 by Brocade Communications Systems, Inc.
   All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
*/

#include "completion_env.h"
#include "connect.h"
#include "error.h"
#include "node.h"
#include "internal.h"
#include "rpc.h"

int configd_node_exists(struct configd_conn *conn, int db, const char *cpath, struct configd_error *error)
{
	int result;
	struct request req = { .fn = "Exists" };

	req.args = json_pack("[iss]", db, conn->session_id, cpath);
	if (!req.args)
		return -1;

	error_init(error, __func__);
	result = get_int(conn, &req, error);
	return result;
}

int configd_node_is_default(struct configd_conn *conn, int db, const char *cpath, struct configd_error *error)
{
	int result;
	struct request req = { .fn = "NodeIsDefault" };

	req.args = json_pack("[iss]", db, conn->session_id, cpath);
	if (!req.args)
		return -1;

	error_init(error, __func__);
	result = get_int(conn, &req, error);
	return result;
}


struct vector *configd_node_get(struct configd_conn *conn, int db, const char *cpath, struct configd_error *error)
{
	struct vector *result;
	struct request req = { .fn = "Get" };

	req.args = json_pack("[iss]", db, conn->session_id, cpath);
	if (!req.args)
		return NULL;

	error_init(error, __func__);
	result = get_vector(conn, &req, error);
	return result;
}

int configd_node_get_status(struct configd_conn *conn, int db, const char *cpath, struct configd_error *error)
{
	int result;
	struct request req = { .fn = "NodeGetStatus" };

	req.args = json_pack("[iss]", db, conn->session_id, cpath);
	if (!req.args)
		return -1;

	error_init(error, __func__);
	result = get_int(conn, &req, error);
	return result;
}

int configd_node_get_type(struct configd_conn *conn, const char *cpath, struct configd_error *error)
{
	int result;
	struct request req = { .fn = "NodeGetType" };

	req.args = json_pack("[ss]", conn->session_id, cpath);
	if (!req.args)
		return -1;

	error_init(error, __func__);
	result = get_int(conn, &req, error);
	return result;
}

char *configd_node_get_comment(struct configd_conn *conn, int db, const char *cpath, struct configd_error *error)
{
	char *result;
	struct request req = { .fn = "NodeGetComment" };

	req.args = json_pack("[iss]", db, conn->session_id, cpath);
	if (!req.args)
		return NULL;

	error_init(error, __func__);
	result = get_str(conn, &req, error);
	return result;
}

static char *configd_tree_get_fn_encoding(struct configd_conn *conn, int db, const char *path, const char *encoding, const char *getFn, unsigned int flags, struct configd_error *error)
{
	char *result;
	struct request req = { .fn = getFn };

	req.args = json_pack("[isss{sbsb}]", db, conn->session_id, path, encoding,
			     "Defaults", !!(flags & CONFIGD_TREEGET_DEFAULTS),
			     "Secrets", !!(flags & CONFIGD_TREEGET_SECRETS));
	if (!req.args)
		return NULL;

	error_init(error, __func__);
	result = get_str(conn, &req, error);
	return result;
}

char *configd_tree_get_encoding_flags(struct configd_conn *conn, int db, const char *path, const char *encoding, unsigned int flags, struct configd_error *error)
{
	return configd_tree_get_fn_encoding(conn, db, path, encoding, "TreeGet",
					    flags & CONFIGD_TREEGET_ALL, error);
}

char *configd_tree_get_encoding(struct configd_conn *conn, int db, const char *path, const char *encoding, struct configd_error *error)
{
	return configd_tree_get_fn_encoding(conn, db, path, encoding, "TreeGet",
					    CONFIGD_TREEGET_ALL, error);
}
char *configd_tree_get(struct configd_conn *conn, int db, const char *path, struct configd_error *error)

{
	return configd_tree_get_encoding(conn, db, path, "json", error);
}

char *configd_tree_get_xml(struct configd_conn *conn, int db, const char *path, struct configd_error *error)
{
	return configd_tree_get_encoding(conn, db, path, "xml", error);
}

char *configd_tree_get_internal(struct configd_conn *conn, int db, const char *path, struct configd_error *error)
{
	return configd_tree_get_encoding(conn, db, path, "internal", error);
}

char *configd_tree_get_full_encoding_flags(struct configd_conn *conn, int db, const char *path, const char *encoding, unsigned int flags, struct configd_error *error)
{
	return configd_tree_get_fn_encoding(conn, db, path, encoding, "TreeGetFull",
					    flags & CONFIGD_TREEGET_ALL, error);
}

char *configd_tree_get_full_encoding(struct configd_conn *conn, int db, const char *path, const char *encoding, struct configd_error *error)
{
	return configd_tree_get_fn_encoding(conn, db, path, encoding, "TreeGetFull",
					    CONFIGD_TREEGET_ALL, error);
}

char *configd_tree_get_full(struct configd_conn *conn, int db, const char *path, struct configd_error *error)
{
	return configd_tree_get_full_encoding(conn, db, path, "json", error);
}

char *configd_tree_get_full_xml(struct configd_conn *conn, int db, const char *path, struct configd_error *error)
{
	return configd_tree_get_full_encoding(conn, db, path, "xml", error);
}

char *configd_tree_get_full_internal(struct configd_conn *conn, int db, const char *path, struct configd_error *error)
{
	return configd_tree_get_full_encoding(conn, db, path, "internal", error);
}

char *configd_node_get_complete_env(struct configd_conn *conn, const char *cpath, struct configd_error *error)
{
	if (!conn || !cpath)
		return NULL;
	// TODO: pass error down
	return getCompletionEnv(conn, cpath);
}
