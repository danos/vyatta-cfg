/*
 * Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
 *
   Copyright (c) 2014 by Brocade Communications Systems, Inc.
   All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
*/

#ifndef CONFIGD_COMPLETION_ENV_H_
#define CONFIGD_COMPLETION_ENV_H_

#ifdef __cplusplus
extern "C" {
#endif

struct configd_conn;

char *getCompletionEnv(struct configd_conn *cstore, const char *path);

#ifdef __cplusplus
}
#endif

#endif
