/*
 * Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
 *
 * Copyright (c) 2014, 2016 by Brocade Communications Systems, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 */

#include <string>
#include <string.h>
#include <vector>

#include <vyatta-util/map.h>
#include <vyatta-util/vector.h>

#include "connect.h"
#include "ctemplate.hpp"
#include "node.h"
#include "rpc.h"
#include "template.h"

static const char *type_map[PRIORITY_TYPE + 2] = {
	[ERROR_TYPE] = "error",
	[INT_TYPE] = "u32",
	[IPV4_TYPE] = "ipv4",
	[IPV4NET_TYPE] = "ipv4net",
	[IPV6_TYPE] = "ipv6",
	[IPV6NET_TYPE] = "ipv6net",
	[MACADDR_TYPE] = "macaddr",
	[DOMAIN_TYPE] = "domain",
	[TEXT_TYPE] = "txt",
	[BOOL_TYPE] = "bool",
	[PRIORITY_TYPE] = "priority",
};

static vtw_type_e map_type(const char *type_str)
{
	if (!type_str)
		return ERROR_TYPE;
	for (int type = INT_TYPE; type_map[type]; ++type)
		if (strcmp(type_str, type_map[type]) == 0)
			return (vtw_type_e)type;
	return ERROR_TYPE;
}


Ctemplate::Ctemplate(struct configd_conn *cstore, const char *path)
	: _cstore(cstore), _path(path), _type(ERROR_TYPE), _type2(ERROR_TYPE),
	  _have_allowed(false), _is_value(false), _is_multi(false),
	  _have_is_multi(false), _is_tag(false), _have_is_tag(false)
{
	_def = NULL;
}

Ctemplate::~Ctemplate()
{
	map_free(_def);
}

bool Ctemplate::get()
{
	_def = configd_tmpl_get(_cstore, _path.c_str(), NULL);
	if (!_def)
		return NULL;

	const char *type = map_get(_def, "type");
	const char *type2 = map_get(_def, "type2");
	_type = map_type(type);
	_type2 = map_type(type2);

	const char *value = map_get(_def, "is_value");
	_is_value = value && (strcmp(value, "1") == 0);

	value = map_get(_def, "multi");
	_is_multi = value && (strcmp(value, "1") == 0);

	value = map_get(_def, "tag");
	_is_tag = value && (strcmp(value, "1") == 0);

	return _def;
}

bool Ctemplate::isMulti()
{
	return _is_multi;
}

bool Ctemplate::isTag()
{
	return _is_tag;
}

const char *Ctemplate::getTypeName(size_t tnum) const
{
	vtw_type_e type = (tnum == 1) ? _type : _type2;
	return type_map[type];
}

const char *Ctemplate::getAllowed(void)
{
	if (_have_allowed)
		return _allowed.c_str();
	struct vector *v = configd_tmpl_get_allowed(_cstore, _path.c_str(), NULL);
	if (!v)
		return "";
	const char *str = vector_next(v, NULL);
	if (!str)
		goto done;
	_allowed += str;
	while ((str = vector_next(v, str))) {
		_allowed += " ";
		_allowed += str;
	}
done:
	_have_allowed = true;
	return _allowed.c_str();;
}

const char *Ctemplate::getCompHelp(void)
{
	if (!_def)
		return NULL;
	return map_get(_def, "comp_help");
}

const char *Ctemplate::getNodeHelp(void)
{
	if (!_def)
		return NULL;
	return map_get(_def, "help");
}
