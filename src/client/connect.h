/*
 * Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
 *
   Copyright (c) 2013-2014 by Brocade Communications Systems, Inc.
   All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
*/

#ifndef CONFIGD_CONNECT_H_
#define CONFIGD_CONNECT_H_

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

struct configd_conn {
	int fd;
	FILE *fp;
	char *session_id;
	unsigned int req_id;
};

/**
 * configd_open_connection connects to configd. The state for this connection is
 * stored in the configd_conn struct, which must be non NULL. The return values are
 * 0:ok, -1:error.
 */
int configd_open_connection(struct configd_conn *);

/**
 * configd_close_connection ends the connection with configd. After this is
 * called the connection struct is no longer valid.
 */
void configd_close_connection(struct configd_conn *);

/**
 * configd_set_session_id sets the session id for this connection. Typically
 * this is done just after connecting to configd. The session id is a string
 * consisting of numbers representing the connection. Typically the value
 * that is used is the pid of the process. Any unique identifier will work
 * as a connection id.
 */
int configd_set_session_id(struct configd_conn *, const char *);

#ifdef __cplusplus
}
#endif

#endif
