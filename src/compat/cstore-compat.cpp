/*
 * Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
 *
 * Copyright (c) 2014-2015 by Brocade Communications Systems, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 */
#include <string.h>
#include <stdarg.h>
#include <uriparser/Uri.h>

#include <map>

#include <vyatta-util/map.h>
#include <vyatta-util/vector.h>

#include "cstore-compat.hpp"

#include "connect.h"
#include "error.h"
#include "node.h"
#include "session.h"
#include "template.h"
#include "transaction.h"
#include "rpc.h"

extern "C" void Perl_croak_nocontext(const char* pat, ...)
__attribute__((noreturn))
__attribute__((format(__printf__,1,2)))
__attribute__((nonnull(1)));

extern "C" void* Perl_get_context(void)
__attribute__((warn_unused_result));

/* 
 *  URI_MULT is defined by the uriparser documenation to need to be 6 when expanding breaks
 *  http://uriparser.sourceforge.net/doc/html/Uri_8h.html#a0d501e7c2a6abe76b729ea72e123dea8 
 */
#define URI_MULT 6

using namespace cstore;

struct configd_conn *Cstore::conn = NULL;
bool Cstore::conn_established = false;

Cstore::Cstore() {
	/*We can't fail in a consturctor
 	  Store weather the connection was successful for use later*/
	if (conn != NULL && conn_established) {
		return;
	}
	connect();
}

Cstore::~Cstore() {
}

void Cstore::connect() {
	conn = (struct configd_conn *)malloc(sizeof(struct configd_conn));
	if (!conn)
		return;
	if (!conn || (configd_open_connection(conn) == -1)) {
		conn_established = false;
	} else {
		char *sid = getenv("VYATTA_CONFIG_SID");
		if (sid != NULL) {
			configd_set_session_id(conn, sid);
		}
		conn_established = true;
	}
}

void Cstore::disconnect() {
	if (conn_established) {
		conn_established = false;
		configd_close_connection(conn);
		conn = NULL;
	}
}

bool Cstore::cfgGetTree(StringVector &path, std::string &result, bool active_cfg) {
	if (!conn_established) {
		return false;
	}

	int db = RUNNING;
	if (!active_cfg) {
		assertSession(__func__);
		db = CANDIDATE;
	}

	char *cpath = strVecToChar(path);
	struct configd_error err = {0};
    char *tree = configd_tree_get_internal(conn, db, cpath, &err);
	if (tree == NULL) {
    	free(cpath);
		return false;
	}
    free(cpath);
	result = std::string(tree);
	free(tree);
	return true;
}

bool Cstore::cfgPathGetType(StringVector &path, std::string &result) {
	if (!conn_established) {
		return false;
	}
	char *cpath = strVecToChar(path);
	int type = configd_node_get_type(conn, cpath, NULL);
	free(cpath);
	switch (type) {
	case NODE_TYPE_LEAF:
		result = "leaf";
		break;
	case NODE_TYPE_MULTI:
		result = "multi";
		break;
	case NODE_TYPE_CONTAINER: 	
		result = "container";
		break;
	case NODE_TYPE_TAG:
		result = "tag";
		break;
	}
	return true;
}

bool Cstore::cfgPathExists(StringVector &path, bool active_cfg) {
	return this->nodeExists(path, active_cfg, false);
}
bool Cstore::cfgPathEffective(StringVector &path) {
	return this->nodeExists(path, false, true);
}
bool Cstore::cfgPathDefault(StringVector &path, bool active_cfg) {
	if (!conn_established) {
		return NULL;
	}
	int db = RUNNING;
	if (!active_cfg) {
		assertSession(__func__);
		db = CANDIDATE;
	}
	char *cpath = strVecToChar(path);
	int result = configd_node_is_default(conn, db, cpath, NULL);
	free(cpath);
	if (result <= 0) {
		return false;
	} else {
		return true;
	}
}

void Cstore::cfgPathGetChildNodes(StringVector &path, StringVector &result, bool active_cfg) {
	std::string type;
	if (cfgPathGetType(path, type)) {
		if (type == "leaf" || type == "multi") {
			return;
		}
	}
	this->nodeGet(path, result, active_cfg, false);
}
bool Cstore::cfgPathGetValue(StringVector &path, std::string &result, bool active_cfg) {
	StringVector sv;
	if (!this->nodeGet(path, sv, active_cfg, false)) {
		return false;
	}
	if (sv.size() > 0) {
		std::string type;
		if (cfgPathGetType(path, type)) {
			if (type != "leaf") {
				return false;
			}
		}
		result = sv.front();
		return true;
	}
	return false; 
}
bool Cstore::cfgPathGetValues(StringVector &path, StringVector &result, bool active_cfg) {
	std::string type;
	if (cfgPathGetType(path, type)) {
		if (type != "multi") {
			return false;
		}
	}

	return this->nodeGet(path, result, active_cfg, false);
}
bool Cstore::cfgPathGetComment(StringVector &path, std::string &result, bool active_cfg) {
	char * cpath = strVecToChar(path);
	int db = RUNNING;
	if (!active_cfg) {
		assertSession(__func__);
		db = CANDIDATE;
	}
	struct configd_error err;
	char * res = configd_node_get_comment(conn, db, cpath, &err);
	if (err.text != NULL) {
		free(cpath);
		return false;
	}
	free(cpath);
	result = std::string(res);
	return true;
}

void Cstore::cfgPathGetEffectiveChildNodes(StringVector &path, StringVector &result) {
	std::string type;
	if (cfgPathGetType(path, type)) {
		if (type == "leaf" || type == "multi") {
			return;
		}
	}
	this->nodeGet(path, result, false, true);
}
bool Cstore::cfgPathGetEffectiveValue(StringVector &path, std::string &result) {
	StringVector sv;
	if (!this->nodeGet(path, sv, false, true)) {
		return  false;
	}
	if (sv.size() > 0) {
		std::string type;
		if (cfgPathGetType(path, type)) {
			if (type != "leaf") {
				return false;
			}
		}
		result = sv.front();
		return true;
	}
	return false;
}
bool Cstore::cfgPathGetEffectiveValues(StringVector &path, StringVector &result) {
	std::string type;
	if (cfgPathGetType(path, type)) {
		if (type != "multi") {
			return false;
		}
	}

	return this->nodeGet(path, result, false, true);
}

bool Cstore::cfgPathDeleted(StringVector &path) {
	return this->nodeIsStatus(path, NODE_STATUS_DELETED);
}
bool Cstore::cfgPathAdded(StringVector &path) {
	return this->nodeIsStatus(path, NODE_STATUS_ADDED);
}
bool Cstore::cfgPathChanged(StringVector &path) {
	return this->nodeIsChanged(path);
}
bool Cstore::cfgPathStatus(StringVector &path, std::string &result) {
	if (!conn_established) {
		return false;
	}
	assertSession(__func__);
	char *cpath = strVecToChar(path);
	NodeStatus status = (NodeStatus)configd_node_get_status(conn, CANDIDATE, cpath, NULL);
	free(cpath);
	switch (status) {
	case NODE_STATUS_UNCHANGED:
		result = "static";
		break;
	case NODE_STATUS_CHANGED:
		result = "changed";
		break;
	case NODE_STATUS_ADDED:
		result = "added";
		break;
	case NODE_STATUS_DELETED:
		result = "deleted";
		break;
	}
	return true;
}
void Cstore::cfgPathGetChildNodesStatus(StringVector &path, StringMap &result) {
	StringVector avec, wvec;
	std::map<std::string, bool> umap;

	nodeGetDB(RUNNING, path, avec);
	nodeGetDB(CANDIDATE, path, wvec);

	for (StringVector::iterator it = avec.begin(); it != avec.end(); it++)
		umap[*it] = true;
	for (StringVector::iterator it = wvec.begin(); it != wvec.end(); it++)
		umap[*it] = true;

	assertSession(__func__);
	StringVector ppath = StringVector(path);
	for (std::map<std::string, bool>::iterator it = umap.begin(); it != umap.end(); it++) {
		std::string c = (*it).first;
		ppath.push_back(c);
		char *cppath = strVecToChar(ppath);
		int status = configd_node_get_status(conn, CANDIDATE, cppath, NULL);
		free(cppath);
		switch (status) {
		case NODE_STATUS_CHANGED:
			result[c] = "changed";
			break;
		case NODE_STATUS_ADDED:
			result[c] = "added";
			break;
		case -1: //error == deleted (not exists in candidate)
		case NODE_STATUS_DELETED:
			result[c] = "deleted";
			break;
		default:
		case NODE_STATUS_UNCHANGED:
			result[c] = "static";
			break;
		}
		ppath.pop_back();
	}
}

void Cstore::cfgPathGetDeletedChildNodes(StringVector &path, StringVector &result) {
	return cfgPathGetDeletedValues(path, result);
}
void Cstore::cfgPathGetDeletedValues(StringVector &path, StringVector &result) {
	StringVector avec, svec;
	if (!nodeGetDB(RUNNING, path, avec)) {
		return;
	}
	if (!nodeGetDB(CANDIDATE, path, svec)) {
		return;
	}
	for (StringVector::iterator ait = avec.begin(); ait != avec.end(); ait++) { 
		bool found = false;
		for (StringVector::iterator sit = svec.begin();
		     sit != svec.end(); sit++) {
			if ((*sit) == (*ait)) {
				found = true;
			}
		}
		if (!found) {
			result.push_back(*ait);
		}
	}
	return;
}

bool Cstore::getParsedTmpl(StringVector &path, StringMap &tmap, bool allow_val) {
	if (!conn_established) {
		return false;
	}
	char *cpath = strVecToChar(path);
	struct map *ctmpl = configd_tmpl_get(conn, cpath, NULL);
	if (ctmpl == NULL) {
		return false;
	}
	free(cpath);
	for (const char *next = NULL; (next = map_next(ctmpl, next)); ) {
		// Split entry into key and value
		std::string entry(next), key, value;
		size_t pos = entry.find("=");
		if (pos == std::string::npos)
			continue;
		key = entry.substr(0, pos);
		value = entry.substr(pos + 1, std::string::npos);
		tmap[key] = value;
	}
	map_free(ctmpl);
	return true;
}
void Cstore::tmplGetChildNodes(StringVector &path, StringVector &cnodes) {
	if (!conn_established) {
		return;
	}
	char *cpath = strVecToChar(path);
	struct vector *v = configd_tmpl_get_children(conn, cpath, NULL);
	free(cpath);
	vecToStrVec(v, cnodes);
	vector_free(v);
	return;
}
bool Cstore::validateTmplPath(StringVector &path, bool validate_vals) {
	if (!conn_established) {
		return NULL;
	}
	char *cpath = strVecToChar(path);
	int result = configd_tmpl_validate_path(conn, cpath, NULL);
	free(cpath);
	if (result <= 0) {
		return false;
	} else {
		return true;
	}
}
bool Cstore::sessionChanged() {
	assertSession(__func__);
	int result = configd_sess_changed(conn, NULL);
	if (result <= 0) {
		return false;
	} else {
		return true;
	}
}
bool Cstore::inSession() {
	int result = configd_sess_exists(conn, NULL);
	if (result <= 0) {
		return false;
	} else {
		return true;
	}
}
bool Cstore::loadFile(char *filename) {
	assertSession(__func__);
	int result = configd_load(conn, filename, NULL);
	if (result == 0) {
		return false;
	} else {
		return true;
	}
}

/*Private functions*/
void Cstore::assertSession(const char * func) {
	if (configd_sess_exists(conn, NULL) != 1) {
		exit_err("calling %s() without config session", func);
	}
}

void Cstore::vecToStrVec(struct vector *v, StringVector &sv) {
	for (const char *str = NULL; (str = vector_next(v, str)); ) {
		sv.push_back(str);
	}
	return;
}

char *Cstore::strVecToChar(StringVector &sv) {
	std::string path = "/";
	switch (sv.size()) {
	case 0:
		break;
	case 1:
		path = sv[0];
		break;
	default:
		for (size_t i = 0; i < sv.size()-1; i++) {
			char *cstr = (char *)malloc(sv[i].size() * URI_MULT);
			if (!cstr) return NULL;
			uriEscapeA(sv[i].c_str(), cstr, URI_FALSE, URI_TRUE);
			path = path + std::string(cstr) + "/";
			free(cstr);
		}
		char *cstr = (char *)malloc(sv[sv.size()-1].size() * URI_MULT);
		if (!cstr) return NULL;
		uriEscapeA(sv[sv.size()-1].c_str(), cstr, URI_FALSE, URI_TRUE);
		path += std::string(cstr);
		free(cstr);
	}
	
	return strdup(path.c_str());
}

bool Cstore::nodeIsChanged(StringVector &path) {
	if (!conn_established) {
		return NULL;
	}
	assertSession(__func__);
	char *cpath = strVecToChar(path);
	NodeStatus result = (NodeStatus)configd_node_get_status(conn, CANDIDATE, cpath, NULL);
	free(cpath);
	return (result == NODE_STATUS_ADDED || result == NODE_STATUS_DELETED || result == NODE_STATUS_CHANGED);
}

bool Cstore::nodeIsStatus(StringVector &path, NodeStatus stat) {
	if (!conn_established) {
		return NULL;
	}
	assertSession(__func__);
	char *cpath = strVecToChar(path);
	if (configd_node_get_status(conn, CANDIDATE, cpath, NULL) == stat) {
		free(cpath);
		return true;
	} else {
		free(cpath);
		return false;
	}
}

bool Cstore::nodeGetDB(int Db, StringVector &args, StringVector &result) {
	if (!conn_established) {
		return false;
	}

	char *path = strVecToChar(args);
	struct vector *v;
	v = configd_node_get(conn, Db, path, NULL);
	free(path);
	if (v == NULL) {
		return false;
	}

	vecToStrVec(v, result); 
	vector_free(v);
	return true;
}

bool Cstore::nodeGet(StringVector &args, StringVector &result, bool active, bool effective) {
	int db = RUNNING;
	if (active) {
	 	db = RUNNING;
	} else if (effective) {
		db = EFFECTIVE; 
	} else {
		assertSession(__func__);
	 	db = CANDIDATE;
	}
	return nodeGetDB(db, args, result);
}

bool Cstore::nodeExistsDB(int Db, StringVector &path) {
	if (!conn_established) {
		return false;
	}

	char *cpath = strVecToChar(path);
	int result;
	result = configd_node_exists(conn, Db, cpath, NULL);
	free(cpath);
	
	if (result <= 0) {
		return false;
	} else {
		return true;
	}
}

bool Cstore::nodeExists(StringVector &path, bool active, bool effective) {
	int db = RUNNING;
	if (active) {
	 	db = RUNNING;
	} else if (effective) {
		db = EFFECTIVE;
	} else {
		assertSession(__func__);
	 	db = CANDIDATE;
	}
	return nodeExistsDB(db, path);
}

/*Mimic cstore*/
void Cstore::exit_err(const char *fmt, ...) {
	va_list alist;
	va_start(alist, fmt);
	vexit_err(fmt, alist);
	va_end(alist);
}
void Cstore::vexit_err(const char *fmt, va_list alist) {
	char buf[256];
	vsnprintf(buf, 256, fmt, alist);
	if (Perl_get_context()) {
		Perl_croak_nocontext("%s", buf);
	} else {
		fprintf(stderr, "%s\n", buf);
		exit(1);
	}
}
