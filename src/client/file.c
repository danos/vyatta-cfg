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
#include "file.h"
#include "internal.h"
#include "rpc.h"

char *configd_file_read(struct configd_conn *conn, const char *filename, struct configd_error *error)
{
	char *result;
	struct request req = { .fn = "ReadConfigFile" };

	req.args = json_pack("[s]", filename);
	if (!req.args)
		return NULL;

	error_init(error, __func__);
	result = get_str(conn, &req, error);
	return result;

}

char *configd_file_migrate(struct configd_conn *conn, const char *filename, struct configd_error *error)
{
	char *result;
	struct request req = { .fn = "MigrateConfigFile" };

	req.args = json_pack("[s]", filename);
	if (!req.args)
		return NULL;

	error_init(error, __func__);
	result = get_str(conn, &req, error);
	return result;

}
