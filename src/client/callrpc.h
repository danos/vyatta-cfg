/*
 * Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
 *
 * Copyright (c) 2014-2015 by Brocade Communications Systems, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 */

#ifndef CONFIGD_CALLRPC_H_
#define CONFIGD_CALLRPC_H_

#ifdef __cplusplus
extern "C" {
#endif

struct configd_conn;
struct configd_error;

/**
 * configd_call_rpc
 *
 * Takes a namespace, rpc name, and the input to the RPC in JSON
 * format.
 *
 * Returns the output of the RPC in JSON format.
 *
 * On error, returns NULL and if the error struct pointer is non-NULL
 * the error will be filled out.
 */
char *configd_call_rpc(struct configd_conn *conn,
		       const char *ns, const char *name,
		       const char *input, struct configd_error *error);

/**
 * configd_call_rpc_xml
 *
 * Takes a namespace, rpc name, and the input to the RPC in XML
 * format.
 *
 * Returns the output of the RPC in XML format.
 *
 * On error, returns NULL and if the error struct pointer is non-NULL
 * the error will be filled out.
 */
char *configd_call_rpc_xml(struct configd_conn *conn,
			   const char *ns, const char *name,
			   const char *input, struct configd_error *error);

#ifdef __cplusplus
}
#endif


#endif
