/*
 * Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
 *
   Copyright (c) 2013-2014 by Brocade Communications Systems, Inc.
   All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
*/

#ifndef CONFIGD_INTERNAL_H_
#define CONFIGD_INTERNAL_H_

#include <stdarg.h>
#include <jansson.h>

#ifdef __cplusplus
extern "C" {
#endif

struct configd_error;
struct configd_conn;
struct map;
struct vector;

typedef struct request {
	const char *fn;
	json_t	*args;
} Request;


struct response {
	int type;
	union {
		char *str_val;
		struct vector *v;
		struct map *m;
		int int_val;
	} result;
	unsigned int id;
};

void error_init(struct configd_error *error, const char *source);
int error_setf(struct configd_error *error, const char *msg, ...);
int error_vsetf(struct configd_error *error, const char *msg, va_list ap);

void response_free(struct response *);
int send_request(struct configd_conn *, const struct request *);
int recv_response(struct configd_conn *, struct response *);
/* Helpers to get specific response types */
char *get_str(struct configd_conn *, struct request *, struct configd_error *);
int get_int(struct configd_conn *, struct request *, struct configd_error *);
struct vector *get_vector(struct configd_conn *, struct request *, struct configd_error *);
struct map *get_map(struct configd_conn *, struct request *, struct configd_error *);
#ifdef __cplusplus
}
#endif

#endif
