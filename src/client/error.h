/*
 * Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
 *
   Copyright (c) 2013-2014 by Brocade Communications Systems, Inc.
   All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
*/

#ifndef CONFIGD_ERROR_H_
#define CONFIGD_ERROR_H_

#ifdef __cplusplus
extern "C" {
#endif

///The configd_error struct consists of the error message text and the function it was generated in (source).
struct configd_error {
	const char *source;
	char *text;
};

///configd_error_free frees the error struct and should be used when the error message is no longer needed.
void configd_error_free(struct configd_error *error);

#ifdef __cplusplus
}
#endif

#endif
