/*
 * Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
 *
 * Copyright (C) 2010 Vyatta, Inc.
 *
 * Copyright (c) 2014-2015 by Brocade Communications Systems, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 */

#include <cstring>
#include <cstdlib>
#include <vector>
#include <string>

#include <connect.h>
#include <transaction.h>
#include <error.h>
#include <node.h>
#include <rpc.h>
#include <ctemplate.hpp>
#include "cli_objects.h"
#include "cpath.hpp"
#include "cstore-varref.hpp"
#include "cstore-c.h"

/* get the value string that corresponds to specified variable ref string.
 *   ref_str: var ref string (e.g., "./cost/@").
 *   type: (output) the node type.
 *   from_active: if true, value string should come from "active config".
 *                otherwise from "working config".
 * return non-zero if successful .
 * otherwise return zero.
 */
int
cstore_get_var_ref(void *handle, const char *ref_str, vtw_type_e *type,
                   char **val, int *ismulti, int from_active)
{
	if (!handle)
		return 0;

	Cpath orig_cfg_path(get_cfg_path());
	Cpath cpath(orig_cfg_path);
	Ctemplate tmpl((struct configd_conn *)handle, get_cfg_path());
	if (!tmpl.get()) {
		return 0;
	}
	if (tmpl.isValue()) {
		cpath.pop();
	}
	set_cfg_path(cpath.to_path_string().c_str());

	std::string vr(ref_str);
	bool bFromActive(from_active);
	VarRef vref(handle, vr, (bool *)ismulti, bFromActive);

	std::string value;
	vtw_type_e t;
	if (!vref.getValue(value, *ismulti, t)) {
		set_cfg_path(orig_cfg_path.to_path_string().c_str());
		return 0;
	}

	*type = t;
	/* follow original implementation. caller is supposed to free this. */
	*val = strdup(value.c_str());
	set_cfg_path(orig_cfg_path.to_path_string().c_str());
	return !!*val;
}

/* set the node corresponding to specified variable ref string to specified
 * value.
 *   ref_str: var ref string (e.g., "../encrypted-password/@").
 *   value: value to be set.
 *   to_active: if true, set in "active config" (not supported).
 *              otherwise in "working config".
 * return non-zero if successful. otherwise return zero.
 *
 * XXX functions in cli_new only performs "set var ref" operations (e.g.,
 *     '$VAR(@) = ""', which sets current node's value to empty string)
 *     during "commit", i.e., if a "set var ref" is specified in
 *     "syntax:", it will not be performed during "set" (but will be
 *     during commit).
 *
 * - the behavior here follows the original implementation and
 *   has these limitations:
 *     * it only supports only single-value leaf nodes.
 */
int
cstore_set_var_ref(void *handle, const char *ref_str, const char *value, int to_active)
{
	if (!handle || to_active)
		return 0;

	int ret = 0;
	char *result;
	int type;
	struct configd_conn *h = static_cast<struct configd_conn *>(handle);

	// Since we only support single value leaf nodes, we can
	// assume that the global cfg_path has the value at the
	// end. We need to pop the value off while we process the
	// VARREF.
	Cpath orig_cfg_path(get_cfg_path());
	Cpath cpath(orig_cfg_path);
	cpath.pop();
	set_cfg_path(cpath.to_path_string().c_str());
	std::string vr(ref_str);
	VarRef vref(handle, ref_str, NULL, to_active);

	if (!vref.getSetPath(cpath))
		goto done;

	type = configd_node_get_type(h, cpath.to_path_string().c_str(), NULL);
	if (type != NODE_TYPE_LEAF)
		goto done;

	cpath += value;
	struct configd_error err;
	result = configd_set(h, cpath.to_path_string().c_str(), &err);
	ret = result != NULL;
	if (result) {
		puts(result);
		free(result);
	} else {
		puts(err.text);
		configd_error_free(&err);
	}
done:
	set_cfg_path(orig_cfg_path.to_path_string().c_str());
	return ret;
}
