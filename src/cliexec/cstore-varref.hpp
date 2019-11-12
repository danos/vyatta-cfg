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

#ifndef _CSTORE_VARREF_H_
#define _CSTORE_VARREF_H_
#include <vector>
#include <string>
#include <map>

#include <connect.h>

#include "cli_objects.h"
#include "cpath.hpp"

class VarRef
{
public:
	VarRef(void *cstore, const std::string &ref_str, bool *ismulti, bool active);
	~VarRef() {};

	bool getValue(std::string &value, bool multi, vtw_type_e &def_type);
	bool getSetPath(Cpath &path);

private:
	struct configd_conn *_cstore;
	bool _active;
	bool _absolute;
	std::string _at_string;
	Cpath _orig_path_comps;
	std::vector<std::pair<Cpath, vtw_type_e> > _paths;

	void process_ref(const Cpath &ref_comps,
	                 const Cpath &cur_path_comps, bool *ismulti, vtw_type_e def_type);
};


#endif /* _CSTORE_VARREF_H_ */

