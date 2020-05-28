/*
 * Copyright (c) 2020, AT&T Intellectual Property.
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Common mocking support for unit tests.
 */
#if !defined(__common_mocks_h__)
#define __common_mocks_h__

#include "assert.h"
#include "CppUTestExt/MockSupport_c.h"

#include <jansson.h>

void set_incoming_rpc_json(json_t *incoming);

#endif
