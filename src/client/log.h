/*
 * Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
 *
  Copyright (c) 2013-2014 by Brocade Communications Systems, Inc.

   All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
*/

#ifndef CONFIGD_LOG_H_
#define CONFIGD_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

void msg_use_console(int);
void msg_out(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
void msg_err(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
void msg_dbg(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));

#ifdef __cplusplus
}
#endif

#endif
