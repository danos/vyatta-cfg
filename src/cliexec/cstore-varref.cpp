/*
 * Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
 *
 * Copyright (C) 2010 Vyatta, Inc.
 *
 * Copyright (c) 2014-2016 by Brocade Communications Systems, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 */

#include <cstdio>
#include <memory>
#include <vector>
#include <string>

#include <vyatta-util/map.h>
#include <vyatta-util/vector.h>
#include <rpc.h>
#include <connect.h>
#include <cpath.hpp>
#include <ctemplate.hpp>
#include <template.h>
#include <node.h>
#include "../cli_cstore.h"
#include "cstore-varref.hpp"

/* constructors/destructors */
VarRef::VarRef(void *cstore, const std::string &ref_str, bool *ismulti, bool active)
	: _cstore(NULL), _active(active), _absolute(false)
{
	/* NOTE: this class will change the paths in the cstore. caller must do
	 *       save/restore for the cstore if necessary.
	 */
	if (!cstore) {
		/* no cstore */
		return;
	}
	_cstore = static_cast<struct configd_conn *>(cstore);
	_absolute = (ref_str[0] == '/');
	std::vector<std::string> tmp;
	Cpath cfg_path(get_cfg_path());
	while (!_absolute && !cfg_path.at_root()) {
		std::string last;
		cfg_path.pop(last);
		tmp.push_back(last);
	}
	while (tmp.size() > 0) {
		_orig_path_comps.push(tmp.back());
		tmp.pop_back();
	}
	size_t si = (_absolute ? 1 : 0);
	size_t sn = 0;
	Cpath rcomps;
	while (si < ref_str.length()
	       && (sn = ref_str.find('/', si)) != ref_str.npos) {
		rcomps.push(ref_str.substr(si, sn - si));
		si = sn + 1;
	}
	if (si < ref_str.length()) {
		rcomps.push(ref_str.substr(si));
	}
	/* NOTE: if path ends in '/', the trailing slash is ignored. */

	/* get the "at" string. this is set inside cli_new.c. */
	_at_string = get_at_string();

	/* process ref */
	Cpath pcomps(_orig_path_comps);
	process_ref(rcomps, pcomps, ismulti, ERROR_TYPE);
}


static int get_values(struct configd_conn *conn, const std::string &path, bool active, std::string &vals)
{
	int db = active ? RUNNING : CANDIDATE;
	const char *str = NULL;
	int result = 1;

	struct vector *v = configd_node_get(conn, db, path.c_str(), NULL);
	if (!v)
		return 0;

	if ((str = vector_next(v, str)) != NULL) {
		vals += str;
		while ((str = vector_next(v, str))) {
			vals += " ";
			vals += str;
		}
	} else {
		result = 0;
	}

	vector_free(v);
	return result;
}


/* process the reference(s).
 * this is a recursive function and always keeps the cstore paths unchanged
 * between invocations.
 *
 * note: def_type is added into _paths along with the paths. when it's
 *       ERROR_TYPE, it means the path needs to be checked for existence.
 *       otherwise, the path is a "value" (or "values") read from the
 *       actual config (working or active).
 */
void
VarRef::process_ref(const Cpath &ref_comps,
                            const Cpath &cur_path_comps,
			    bool *ismulti,
                            vtw_type_e def_type)
{
	if (ref_comps.size() == 0) {
		/* done */
		_paths.push_back(std::pair<Cpath, vtw_type_e>(cur_path_comps, def_type));
		return;
	}

	Cpath rcomps;
	Cpath pcomps(cur_path_comps);
	std::string cr_comp = ref_comps[0];
	for (size_t i = 1; i < ref_comps.size(); i++) {
		rcomps.push(ref_comps[i]);
	}

	Ctemplate def(_cstore, pcomps.to_path_string().c_str());
	bool got_tmpl = (def.get() != 0);

	bool handle_leaf = false;
	if (cr_comp == "@") {
		if (!got_tmpl) {
			/* invalid path */
			return;
		}
		if (def.isTypeless()) {
			/* no value for typeless node, so this should be invalid ref
			 * according to the spec.
			 * XXX however, the original implementation erroneously treats
			 * this as valid ref and returns the node "name" as the "value".
			 * for backward compatibility, keep the same behavior.
			 */
			process_ref(rcomps, pcomps, ismulti, ERROR_TYPE);
			return;
		}
		if (pcomps.size() == _orig_path_comps.size()) {
			if (pcomps.size() == 0 || pcomps == _orig_path_comps) {
				/* we are at the original path. this is a self-reference, e.g.,
				 * $VAR(@), so use the "at string".
				 */
				pcomps.push(_at_string);
				process_ref(rcomps, pcomps, ismulti, def.getType(1));
				return;
			}
		}
		if (!def.isSingleLeafNode() && !def.isMultiLeafNode()) {
			if (pcomps.size() < _orig_path_comps.size()) {
				/* within the original path. @ translates to the path comp. */
				pcomps.push(_orig_path_comps[pcomps.size()]);
				process_ref(rcomps, pcomps, ismulti, def.getType(1));
			}
			return;
		}
		/* handle leaf node */
		handle_leaf = true;
	} else if (cr_comp == ".") {
		process_ref(rcomps, pcomps, ismulti, ERROR_TYPE);
	} else if (cr_comp == "..") {
		if (!got_tmpl || pcomps.size() == 0) {
			/* invalid path */
			return;
		}
		pcomps.pop();
		if (pcomps.size() > 0) {
			/* not at root yet */
			Ctemplate parent_def(_cstore, pcomps.to_path_string().c_str());
			if (!parent_def.get()) {
				/* invalid tmpl path */
				return;
			}
			if (parent_def.isTagValue()) {
				/* at "tag value", need to pop one more. */
				if (pcomps.size() == 0) {
					/* invalid path */
					return;
				}
				pcomps.pop();
			}
		}
		process_ref(rcomps, pcomps, ismulti, ERROR_TYPE);
	} else if (cr_comp == "@@") {
		if (!got_tmpl) {
			/* invalid path */
			return;
		}
		if (def.isTypeless()) {
			/* no value for typeless node */
			if (ismulti) {
				*ismulti = true;
			}
			int db = _active ? RUNNING : CANDIDATE;
			struct vector *cnodes = configd_node_get(_cstore, db, pcomps.to_path_string().c_str(), NULL);
			if (!vector_count(cnodes)) {
				vector_free(cnodes);
				return;
			}
			const char *str = NULL;
			while ((str = vector_next(cnodes, str))) {
				pcomps.push(str);
				process_ref(rcomps, pcomps, ismulti, def.getType(1));
				pcomps.pop();
			}
			vector_free(cnodes);
		}
		if (def.isValue()) {
			/* invalid ref */
			return;
		}
		if (def.isTag()) {
			/* tag node */
			if (ismulti) {
				*ismulti = true;
			}
			int db = _active ? RUNNING : CANDIDATE;
			struct vector *cnodes = configd_node_get(_cstore, db, pcomps.to_path_string().c_str(), NULL);
			if (!vector_count(cnodes)) {
				vector_free(cnodes);
				return;
			}
			const char *str = NULL;
			while ((str = vector_next(cnodes, str))) {
				pcomps.push(str);
				process_ref(rcomps, pcomps, ismulti, def.getType(1));
				pcomps.pop();
			}
			vector_free(cnodes);
		} else {
			/* handle leaf node */
			handle_leaf = true;
		}
	} else {
		/* just text. go down 1 level. */
		if (got_tmpl && def.isTagNode()) {
			/* at "tag node". need to go down 1 more level. */
			if (pcomps.size() > _orig_path_comps.size()) {
				/* already under the original node. invalid ref. */
				return;
			} else if (pcomps.size() == _orig_path_comps.size()) {
				/* at the tag value. use the at_string. */
				pcomps.push(_at_string);
			} else  {
				/* within the original path. take the original tag value. */
				pcomps.push(_orig_path_comps[pcomps.size()]);
			}
		}
		pcomps.push(cr_comp);
		process_ref(rcomps, pcomps, ismulti, ERROR_TYPE);
	}

	if (handle_leaf) {
		if (def.isMulti()) {
			/* multi-value node */
			if (ismulti) {
				*ismulti = true;
			}
			std::string val;
			if (!get_values(_cstore, pcomps.to_path_string(), _active, val)) {
				return;
			}
			pcomps.push(val);
			/* treat "joined" multi-values as TEXT_TYPE */
			_paths.push_back(std::pair<Cpath, vtw_type_e>(pcomps, TEXT_TYPE));
			/* at leaf. stop recursion. */
		} else {
			/* single-value node */
			std::string val;
			vtw_type_e t = def.getType(1);
			if (!get_values(_cstore, pcomps.to_path_string(), _active, val)) {
				/* can't get value => treat it as non-existent (empty value
				 * and type ERROR_TYPE)
				 */
				val = "";
				t = ERROR_TYPE;
			}
			pcomps.push(val);
			_paths.push_back(std::pair<Cpath, vtw_type_e>(pcomps, t));
			/* at leaf. stop recursion. */
		}
	}
}

bool
VarRef::getValue(std::string &value, bool multi, vtw_type_e &def_type)
{
	std::vector<std::string> result;
	std::map<std::string, bool> added;
	def_type = ERROR_TYPE;
	for (size_t i = 0; i < _paths.size(); i++) {
		if (_paths[i].first.size() == 0) {
			/* empty path */
			continue;
		}
		if (!multi && added.find(_paths[i].first.back()) != added.end()) {
			/* already added */
			continue;
		}
		if (_paths[i].second == ERROR_TYPE
		    && (configd_node_exists(_cstore, _active ? RUNNING : CANDIDATE,
					    _paths[i].first.to_path_string().c_str(),
					    NULL) != 1)) {
			/* path doesn't exist => empty string */
			added[""] = true;
			result.push_back("");
			continue;
		}
		if (_paths[i].second != ERROR_TYPE) {
			/* set def_type. all types should be the same if multiple entries exist. */
			def_type = _paths[i].second;
		}
		added[_paths[i].first.back()] = true;
		result.push_back(_paths[i].first.back());
	}
	if (result.size() == 0) {
		/* got nothing */
		return false;
	}
	if (result.size() > 1 || def_type == ERROR_TYPE) {
		/* if no type is available or we are returning "joined" multiple values,
		 * treat it as text type.
		 */
		def_type = TEXT_TYPE;
	}
	value = "";
	for (size_t i = 0; i < result.size(); i++) {
		if (i > 0) {
			value += " ";
		}
		value += result[i];
	}
	return true;
}

bool
VarRef::getSetPath(Cpath &path_comps)
{
	if (_paths.size() != 1) {
		/* for set_var_ref operation, there can be only one path. */
		return false;
	}
	path_comps = _paths[0].first;
	/* note that for "varref set" operation, the varref must refer to the
	 * "value" of a single-value leaf node, e.g.,
	 * "$VAR(plaintext-password/@)". so pop the last comp to give the
	 * correct path for "set". the caller is responsible for verifying
	 * whether the path points to a single-value leaf node.
	 */
	path_comps.pop();
	return true;
}

