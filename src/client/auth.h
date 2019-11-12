/*
 * Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
 *
 * Copyright (c) 2014 by Brocade Communications Systems, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 */

#ifndef CONFIGD_AUTH_H_
#define CONFIGD_AUTH_H_

#ifdef __cplusplus
extern "C" {
#endif

struct configd_conn;
struct map;
struct configd_error;

/**
 * configd_auth_getperms returns the matched portion of the auth ruleset for
 * a given user as a map from path to permission. On error the return value
 * is NULL, and if the error strut pointer is non NULL then it will be populated.
 */
struct map *configd_auth_getperms(struct configd_conn *, struct configd_error *error);

/**
 * configd_auth_authorized takes a '/' separated path and permission type and
 * tests that the connected user is authorized to preform the action. The return
 * values are 0:false, 1:true, -1:error. On error if an error struct pointer is
 * non NULL the error will be populated.
 */
int configd_auth_authorized(struct configd_conn *conn, const char *cpath, uint perm, struct configd_error *error);

#ifdef __cplusplus
}
#endif

#endif
