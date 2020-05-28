/*
 * Copyright (c) 2019-2020, AT&T Intellectual Property. All rights reserved.
 *
   Copyright (c) 2013-2014 by Brocade Communications Systems, Inc.
   All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
*/

#ifndef RPC_H_
#define RPC_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	AUTO = 0,
	RUNNING,
	CANDIDATE,
	EFFECTIVE
} Db;

typedef enum {
	NODE_STATUS_UNCHANGED = 0,
	NODE_STATUS_CHANGED,
	NODE_STATUS_ADDED,
	NODE_STATUS_DELETED
} NodeStatus;

typedef enum {
	NODE_TYPE_LEAF = 0,
	NODE_TYPE_MULTI,
	NODE_TYPE_CONTAINER,
	NODE_TYPE_TAG
} NodeType;

typedef enum {
	INIT = 0,
	STRING,
	INT,
	STATUS,
	VECTOR,
	MAP,
	ERROR,
	MGMTERROR
} RespT;

typedef enum {
	SHOWF_DEFAULTS     = 0x01,
	SHOWF_HIDE_SECRETS = 0x02,
	SHOWF_CONTEXT_DIFF = 0x04,
	SHOWF_COMMANDS     = 0x08,
	SHOWF_IGNORE_EDIT  = 0x10
} ShowFlagT;

#ifdef __cplusplus
}
#endif

#endif
