/*
 * Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
 *
  Copyright (c) 2013 by Brocade Communications Systems, Inc.

   All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
*/

#ifndef CONFIGD_FILE_H_
#define CONFIGD_FILE_H_

#ifdef __cplusplus
extern "C" {
#endif

struct configd_conn;
struct configd_error;

char *configd_file_read(struct configd_conn *, const char *filename, struct configd_error *error);
char *configd_file_migrate(struct configd_conn *, const char *filename, struct configd_error *error);

#ifdef __cplusplus
}
#endif


#endif
