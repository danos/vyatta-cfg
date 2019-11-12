/*
 * Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
 *
 * Copyright (c) 2014 by Brocade Communications Systems, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 */

#include <algorithm>
#include <argz.h>
#include <string>
#include <string.h>
#include <vector>
#include <map>

#include <vyatta-util/map.h>
#include <vyatta-util/vector.h>

#include "completion_env.h"
#include "connect.h"
#include "cpath.hpp"
#include "cpath.hpp"
#include "ctemplate.hpp"
#include "node.h"
#include "rpc.h"
#include "template.h"
#include "session.h"

/* shell api vars */
static const std::string C_ENV_SHAPI_COMP_VALS = "_cli_shell_api_comp_values";
static const std::string C_ENV_SHAPI_LCOMP_VAL = "_cli_shell_api_last_comp_val";
static const std::string C_ENV_SHAPI_COMP_HELP = "_cli_shell_api_comp_help";
static const std::string C_ENV_SHAPI_HELP_ITEMS = "_cli_shell_api_hitems";
static const std::string C_ENV_SHAPI_HELP_STRS = "_cli_shell_api_hstrs";

static bool less(const std::pair<std::string, std::string> left, const std::pair<std::string, std::string> right) {
	return left.first < right.first;
}

/* escape the single quotes in the string for shell */
static void shell_escape_squotes(std::string &str)
{
	size_t sq = 0;
	while ((sq = str.find('\'', sq)) != str.npos) {
		str.replace(sq, 1, "'\\''");
		sq += 4;
	}
}

/* return the environment string needed for "completion".
 * return NULL on error
 *
 * note: comps must have at least 2 components, the "command" and the
 *       first path element (which can be empty string).
 */
char *getCompletionEnv(struct configd_conn *cstore, const char *path)
{
	struct map *m;
	char *comps_argz;
	size_t comps_size;
	if (argz_create_sep(path, '/', &comps_argz, &comps_size))
		return NULL;

	size_t comps_len = argz_count(comps_argz, comps_size);
	const char *comp = argz_next(comps_argz, comps_size, NULL);
	if (!comp) {
		free(comps_argz);
		return NULL;
	}
	std::string cmd = comp;
	Cpath pcomps;
	for (size_t i = 1; i < (comps_len - 1); ++i) {
		comp = argz_next(comps_argz, comps_size, comp);
		pcomps.push_unescape(comp);
	}
	comp = argz_next(comps_argz, comps_size, comp);
	std::string last_comp = comp;
	free(comps_argz);

	bool exists_only = (cmd == "delete" || cmd == "show"
	                    || cmd == "comment" || cmd == "activate"
	                    || cmd == "deactivate");

	/* at this point, pcomps contains the command line arguments minus the
	 * "command" and the last one.
	 */
	bool is_typeless = true;
	bool is_leaf_value = false;
	bool is_value = false;
	Ctemplate def(cstore, pcomps.to_path_string().c_str());
	if (pcomps.size() > 0) {
		if (!def.get()) {
			/* invalid path */
			return NULL;
		}
		if (exists_only
		    && configd_node_exists(cstore, CANDIDATE,
					   pcomps.to_path_string().c_str(), NULL) != 1) {
			/* invalid path for the command (must exist) */
			return NULL;
		}
		is_typeless = def.isTypeless();
		is_leaf_value = def.isLeafValue();
		is_value = def.isValue();
	} else {
		/* we are at root. default values simulate a typeless node so nop.
		 * note that in this case def is "empty", so must ensure that it's
		 * not used.
		 */
	}

	/* at this point, cfg and tmpl paths are constructed up to the comp
	 * before last_comp, and def is parsed.
	 */
	if (is_leaf_value) {
		/* invalid path (this means the comp before last_comp is a leaf value) */
		return NULL;
	}

	std::vector<std::string> comp_vals;
	std::string comp_string;
	std::string comp_help;
	std::vector<std::pair<std::string, std::string> > help_pairs;
	std::map<std::string, std::string> cmap;
	bool last_comp_val = true;
	
	//gethelp, add to help_pairs
	m = configd_get_help(cstore, (exists_only ? 0 : 1), pcomps.to_path_string().c_str(), NULL);
	if (m == NULL) {
		return NULL;
	}
	const char *next = NULL;
	size_t pos;
	while ((next = map_next(m, next)) != NULL) {
		std::string entry(next), key, value;
		pos = entry.find("=");
		if (pos == std::string::npos) {
			continue;
		}
		key = entry.substr(0, pos);
		value = entry.substr(pos + 1, entry.length() - pos);
		cmap[key] = value;
	}
	sort(help_pairs.begin(), help_pairs.end(), less);
	if (is_typeless || is_value) {
		/* path so far is at a typeless node OR a tag value (tag already
		 * checked above):
		 *   completions: from tmpl children.
		 *   help:
		 *     values: same as completions.
		 *     text: "help" from child templates.
		 *
		 * note: for such completions, we filter non-existent nodes if
		 *       necessary.
		 *
		 * also, the "root" node case above will reach this block, so
		 * must not use def in this block.
		 */
		/* last comp is not value */
		for (std::map<std::string,std::string>::iterator it=cmap.begin(); it!=cmap.end(); ++it) {
			if (last_comp == "" || it->first.compare(0, last_comp.length(), last_comp) == 0) {
				comp_vals.push_back(it->first);
			}
		}
		if (comp_vals.size() == 0) {
			return NULL;
		}
		sort(comp_vals.begin(), comp_vals.end());
		std::string value, key;
		for (size_t i = 0; i < comp_vals.size(); i++) {
			key = comp_vals[i];
			value = cmap[key];
			std::pair<std::string, std::string> hpair(key, value);
			help_pairs.push_back(hpair);
		}
		last_comp_val = false;
	} else {
		/* path so far is at a "value node".
		 * note: follow the original implementation and don't filter
		 *       non-existent values for such completions
		 *
		 * also, cannot be "root" node if we reach here, so def can be used.
		 */
		for (std::map<std::string,std::string>::iterator it=cmap.begin(); it!=cmap.end(); ++it) {
			std::pair<std::string, std::string> hpair(it->first, it->second);
			help_pairs.push_back(hpair);
			comp_vals.push_back(it->first);
			
		}
		/* more possible completions from this node's template:
		 *   "allowed"
		 */
		if (!exists_only && def.getAllowed()) {
			/* do "allowed".
			 * note: emulate original implementation and set up COMP_WORDS and
			 *       COMP_CWORD environment variables. these are needed by some
			 *       "allowed" scripts.
			 */
			comp_string += def.getAllowed();
		}
		/* now handle help. */
		if (def.getCompHelp()) {
			/* "comp_help" exists. */
			comp_help = def.getCompHelp();
			shell_escape_squotes(comp_help);
		}
	}

	/* from this point on cannot use def (since the "root" node case
	 * can reach here).
	 */
	/* this var is the array of possible completions */
	std::string env = (C_ENV_SHAPI_COMP_VALS + "=(");
	for (size_t i = 0; i < comp_vals.size(); i++) {
		shell_escape_squotes(comp_vals[i]);
		env += ("'" + comp_vals[i] + "' ");
	}
	/* as mentioned above, comp_string is the complete command output.
	 * let the shell eval it into the array since we don't want to
	 * re-implement the shell interpretation here.
	 *
	 * note that as a result, we will not be doing the filtering here.
	 * instead, the completion script will do the filtering on
	 * the resulting comp_values array. should be straightforward since
	 * there's no "existence filtering", only "prefix filtering".
	 */
	env += (comp_string + "); ");

	/* this var indicates whether the last comp is "value"
	 * follow original implementation: if last comp is value, completion
	 * script needs to do the following.
	 *   use comp_help if exists
	 *   prefix filter comp_values
	 *   replace any <*> in comp_values with ""
	 *   convert help items to data representation
	 */
	env += (C_ENV_SHAPI_LCOMP_VAL + "=");
	env += (last_comp_val ? "true; " : "false; ");

	/* this var is the "comp_help" string */
	env += (C_ENV_SHAPI_COMP_HELP + "='" + comp_help + "'; ");

	/* this var is the array of "help items", i.e., type names, etc. */
	std::string hitems = (C_ENV_SHAPI_HELP_ITEMS + "=(");
	/* this var is the array of "help strings" corresponding to the items */
	std::string hstrs = (C_ENV_SHAPI_HELP_STRS + "=(");
	for (size_t i = 0; i < help_pairs.size(); i++) {
		std::string hi = help_pairs[i].first;
		std::string hs = help_pairs[i].second;
		shell_escape_squotes(hi);
		shell_escape_squotes(hs);
		/* get rid of leading/trailing "space" chars in help string */
		while (hi.size() > 0 && isspace(hi[0])) {
			hi.erase(0, 1);
		}
		while (hs.size() > 0 && isspace(hs[0])) {
			hs.erase(0, 1);
		}
		while (hi.size() > 0 && isspace(hi[hi.size() - 1])) {
			hi.erase(hi.size() - 1);
		}
		while (hs.size() > 0 && isspace(hs[hs.size() - 1])) {
			hs.erase(hs.size() - 1);
		}
		hitems += ("'" + hi + "' ");
		hstrs += ("'" + hs + "' ");
	}
	env += (hitems + "); " + hstrs + "); ");
	return strdup(env.c_str());
}
