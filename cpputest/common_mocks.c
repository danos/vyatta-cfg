/*
 * Copyright (c) 2020, AT&T Intellectual Property.
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-only//
 *
 * Unit test mocks and other helper functions.
 */

#include "CppUTest/TestHarness_c.h"
#include "CppUTestExt/MockSupport_c.h"
#include <string.h>

#include "common_mocks.h"
#include "log.h"
#include <syslog.h>

#include <jansson.h>

// Mocks for functions not included in UT build so we can verify they are
// called with correct params, and return relevant params if necessary.

void msg_err(const char *fmt, ...) {
    mock_c()->actualCall("msg_err")->withStringParameters("fmt", fmt);
}

void msg_out(const char *fmt, ...) {
    mock_c()->actualCall("msg_out")->withStringParameters("fmt", fmt);
}

// Without this tests use real feof() and crash.  Not entirely clear how this
// doesn't create duplicate definition given UT builds (but then crashes) if
// it isn't included ...
int feof(FILE *stream) {
    return 0; // Never end of file ...
}

// The following functions wrap the underlying function, thus allowing calls
// to them to be diverted.  See LDFLAGS in makefile.

static json_t *incoming_rpc = NULL;

void set_incoming_rpc_json(json_t *incoming) {
    incoming_rpc = incoming;
}

json_t *__wrap_json_loadf(FILE *input, size_t flags, json_error_t *error) {
    return incoming_rpc;
}

int __wrap_write(FILE *output, char *jstr, size_t jlen) {
    return 0;
}

char *__wrap_json_dumps(const json_t *json, size_t flags) {
    char *dump_str = (char *)calloc(1, 10);
    return dump_str;
}
