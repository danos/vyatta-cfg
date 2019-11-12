/*
 * Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
 *
   Copyright (c) 2013-2014 by Brocade Communications Systems, Inc.
   All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
*/

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "error.h"
#include "internal.h"

void error_init(struct configd_error *error, const char *source)
{
	if (error != NULL) {
		error->text = NULL;
		error->source = source;
	}
}

void configd_error_free(struct configd_error *error)
{
	if (error != NULL) {
		free(error->text);
		error->text = NULL;
		error->source = NULL;
	}
}

int error_setf(struct configd_error *error, const char *msg, ...)
{
	int result;
	va_list ap;

	va_start(ap, msg);
	result = error_vsetf(error, msg, ap);
	va_end(ap);
	return result;
}

int error_vsetf(struct configd_error *error, const char *msg, va_list ap)
{
	if (error == NULL || msg == NULL) {
		errno = EINVAL;
		return -1;
	}

	return vasprintf(&error->text, msg, ap);
}
