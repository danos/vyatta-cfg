/*
 * Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
 *
  Copyright (c) 2013 by Brocade Communications Systems, Inc.

   All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
*/

#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>

#include "log.h"

/* Set if the message functions also generate output to stdout/stderr */
static int use_console_;

void msg_use_console(int use_console)
{
	use_console_ = !!use_console;
}


void msg_out(const char *fmt, ...)
{
	va_list ap;
	if (use_console_) {
		va_start(ap, fmt);
		vfprintf(stdout, fmt, ap);
		va_end(ap);
	}
	va_start(ap, fmt);
	vsyslog(LOG_NOTICE, fmt, ap);
	va_end(ap);
}

void msg_err(const char *fmt, ...)
{
	va_list ap;
	if (use_console_) {
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
	}
	va_start(ap, fmt);
	/* TODO: Make debug for now */
	vsyslog(LOG_DEBUG, fmt, ap);
	va_end(ap);
}

void msg_dbg(const char *fmt, ...)
{
	va_list ap;
	if (use_console_) {
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
	}
        va_start(ap, fmt);
	vsyslog(LOG_DEBUG, fmt, ap);
	va_end(ap);
}

