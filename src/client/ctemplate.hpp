/*
 * Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
 *
 * Copyright (c) 2014 by Brocade Communications Systems, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 */

#ifndef CTEMPLATE_HPP_
#define CTEMPLATE_HPP_

#include <string>

#include "../cli_cstore.h"

struct configd_conn;
struct map;

class Ctemplate
{
public:
	Ctemplate(struct configd_conn *cstore, const char *path);
	~Ctemplate();
	bool get();
	bool isValue() const { return _is_value; };
	bool isTypeless(size_t tnum = 1) const {
		return ((tnum == 1) ? (_type == ERROR_TYPE)
		        : (_type2 == ERROR_TYPE));
	};
	bool isMulti();
	bool isTag();

	bool isTagNode() { return isTag() && !isValue(); };
	bool isTagValue() { return isTag() && isValue(); };
	bool isLeafValue() { return (!isTag() && isValue()); };
	bool isSingleLeafNode() { return !isValue() && !isMulti() && !isTag() && !isTypeless(); };
	bool isMultiLeafNode() { return !isValue() && isMulti(); };

	vtw_type_e getType(size_t tnum = 1) const {
		return ((tnum == 1) ? _type : _type2);
	};
	const char *getTypeName(size_t tnum = 1) const;

	const char *getAllowed();
	const char *getCompHelp();
	const char *getNodeHelp();

private:
	struct configd_conn *_cstore;
	struct map *_def;
	std::string _path;
	std::string _allowed;
	vtw_type_e _type;
	vtw_type_e _type2;
	bool _have_allowed;
	bool _is_value;
	bool _is_multi;
	bool _have_is_multi;
	bool _is_tag;
	bool _have_is_tag;
};

#endif
