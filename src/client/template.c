/*
 * Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
 *
   Copyright (c) 2013-2015 by Brocade Communications Systems, Inc.
   All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
*/

#include "connect.h"
#include "error.h"
#include "template.h"
#include "internal.h"
#include "rpc.h"

char *configd_schema_get(struct configd_conn *conn, const char *module,
			 const char *fmt, struct configd_error *error)
{
	char *result;
	struct request req = { .fn = "SchemaGet" };

	req.args = json_pack("[ss]", module, fmt);
	if (!req.args)
		return NULL;

	error_init(error, __func__);
	result = get_str(conn, &req, error);
	return result;

}

char *configd_get_schemas(struct configd_conn *conn, struct configd_error *error)
{
	char *result;
	struct request req = { .fn = "GetSchemas" };

	req.args = json_pack("[]");
	if (!req.args)
		return NULL;

	error_init(error, __func__);
	result = get_str(conn, &req, error);
	return result;

}

struct map *configd_get_features(struct configd_conn *conn, struct configd_error *error)
{
	struct map *result;
	struct request req = { .fn = "GetFeatures" };

	req.args = json_pack("[]");
	if (!req.args)
		return NULL;

	error_init(error, __func__);
	result = get_map(conn, &req, error);
	return result;

}

struct map *configd_tmpl_get(struct configd_conn *conn, const char *cpath, struct configd_error *error)
{
	struct map *result;
	struct request req = { .fn = "TmplGet" };

	req.args = json_pack("[s]", cpath);
	if (!req.args)
		return NULL;

	error_init(error, __func__);
	result = get_map(conn, &req, error);
	return result;
}

struct vector *configd_tmpl_get_children(struct configd_conn *conn, const char *cpath, struct configd_error *error)
{
	struct vector *result;
	struct request req = { .fn = "TmplGetChildren" };

	req.args = json_pack("[s]", cpath);
	if (!req.args)
		return NULL;

	error_init(error, __func__);
	result = get_vector(conn, &req, error);
	return result;
}

struct vector *configd_tmpl_get_allowed(struct configd_conn *conn, const char *cpath, struct configd_error *error)
{
	struct vector *result;
	struct request req = { .fn = "TmplGetAllowed" };
	
	char *sid = "RUNNING";
	if (conn->session_id) {
		sid = conn->session_id;
	}

	req.args = json_pack("[ss]", sid, cpath);
	if (!req.args)
		return NULL;

	error_init(error, __func__);
	result = get_vector(conn, &req, error);
	return result;
}

int configd_tmpl_validate_path(struct configd_conn *conn, const char *cpath, struct configd_error *error)
{
	int result;
	struct request req = { .fn = "TmplValidatePath" };

	req.args = json_pack("[s]", cpath);
	if (!req.args)
		return -1;

	error_init(error, __func__);
	result = get_int(conn, &req, error);
	return result;
}

int configd_tmpl_validate_values(struct configd_conn *conn, const char *cpath, struct configd_error *error)
{
	int result;
	struct request req = { .fn = "TmplValidateValues" };

	req.args = json_pack("[s]", cpath);
	if (!req.args)
		return -1;

	error_init(error, __func__);
	result = get_int(conn, &req, error);
	return result;
}
