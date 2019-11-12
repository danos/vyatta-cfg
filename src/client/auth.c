/*
 * Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
 *
   Copyright (c) 2013-2014 by Brocade Communications Systems, Inc.
   All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
*/

#include "connect.h"
#include "error.h"
#include "auth.h"
#include "internal.h"
#include "rpc.h"

struct map *configd_auth_getperms(struct configd_conn *conn, struct configd_error *error)
{
	struct map *result;
	struct request req = { .fn = "AuthGetPerms" };

	req.args = json_pack("[]");
	if (!req.args)
		return NULL;

	error_init(error, __func__);
	result = get_map(conn, &req, error);
	return result;
}

int configd_auth_authorized(struct configd_conn *conn, const char *cpath, uint perm, struct configd_error *error)
{
	int result;
	struct request req = { .fn = "AuthAuthorize" };

	req.args = json_pack("[si]", cpath, perm);
	if (!req.args)
		return -1;

	error_init(error, __func__);
	result = get_int(conn, &req, error);
	return result;
}
