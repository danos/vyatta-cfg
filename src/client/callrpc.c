/*
 * Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
 *
 * Copyright (c) 2014-2015 by Brocade Communications Systems, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 */

#include "connect.h"
#include "error.h"
#include "callrpc.h"
#include "internal.h"

static char *call_rpc(struct configd_conn *conn, const char *encoding,
		       const char *ns, const char *name,
		       const char *input, struct configd_error *error)
{
	char *result;
	struct request req = { .fn = "CallRpc" };

	req.args = json_pack("[ssss]", ns, name, input, encoding);
	if (!req.args)
		return NULL;

	error_init(error, __func__);
	result = get_str(conn, &req, error);
	return result;
}

char *configd_call_rpc(struct configd_conn *conn,
		       const char *ns, const char *name,
		       const char *input, struct configd_error *error)
{
	return call_rpc(conn, "json", ns, name, input, error);
}

char *configd_call_rpc_xml(struct configd_conn *conn,
			   const char *ns, const char *name,
			   const char *input, struct configd_error *error)
{
	return call_rpc(conn, "xml", ns, name, input, error);
}
