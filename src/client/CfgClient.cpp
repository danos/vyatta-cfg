/*
 * Copyright (c) 2018-2021, AT&T Intellectual Property Inc. All rights reserved.
 * Copyright (c) 2015-2016 by Brocade Communications Systems, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 */
#include <unistd.h>
#include <uriparser/Uri.h>

#include <vyatta-util/map.h>
#include <vyatta-util/vector.h>

#include "rpc.h"
#include "connect.h"
#include "error.h"
#include "session.h"
#include "auth.h"
#include "node.h"
#include "transaction.h"
#include "template.h"
#include "callrpc.h"
#include "CfgClient.hpp"

/*
 *  URI_MULT is defined by the uriparser documenation to need to be 6 when expanding breaks
 *  http://uriparser.sourceforge.net/doc/html/Uri_8h.html#a0d501e7c2a6abe76b729ea72e123dea8
 */
#define URI_MULT 6

typedef int (intapi)(struct configd_conn *, struct configd_error *);
typedef int (intapistr)(struct configd_conn *, const char *, struct configd_error *);
typedef int (intapiintstr)(struct configd_conn *, int, const char *, struct configd_error *);
typedef char *(strapi)(struct configd_conn *, struct configd_error *);
typedef char *(strapistr)(struct configd_conn *, const char *, struct configd_error *);
typedef char *(strapiintstr)(struct configd_conn *, int, const char *, struct configd_error *);
typedef char *(strapiintstrstr)(struct configd_conn *, int, const char *, const char *, struct configd_error *);
typedef struct vector *(vecapistr)(struct configd_conn *, const char *, struct configd_error *);
typedef struct vector *(vecapiintstr)(struct configd_conn *, int, const char *, struct configd_error *);
typedef struct map *(mapapi)(struct configd_conn *, struct configd_error *);
typedef struct map *(mapapistr)(struct configd_conn *, const char *, struct configd_error *);
typedef struct map *(mapapiintstr)(struct configd_conn *, int, const char *, struct configd_error *);

static std::string url_escape(const std::string &s)
{
	char *out = new char[s.length() * URI_MULT];
	uriEscapeA(s.c_str(), out, URI_FALSE, URI_TRUE);
	std::string outs = out;
	delete[] out;
	return outs;
}

static std::string mkpath(const std::vector<std::string> &cfgpath)
{
	std::string path("/");
	for (size_t i = 0; i < cfgpath.size(); ++i) {
		if (i) {
			path += "/";
		}
		path += url_escape(cfgpath[i]);
	}
	return path;
}


static int callintapi(struct configd_conn *_conn, intapi *api) throw(CfgClientException)
{
	struct configd_error err = { 0, };
	int result = api(_conn, &err);
	if (result == -1) {
		std::string msg(err.text);
		configd_error_free(&err);
		throw(CfgClientException(msg));
	}
	configd_error_free(&err);
	return result;
}

static int callintapi(struct configd_conn *_conn, intapistr *api, const std::string &str) throw(CfgClientException)
{
	struct configd_error err = { 0, };
	int result = api(_conn, str.c_str(), &err);
	if (result == -1) {
		std::string msg(err.text);
		configd_error_free(&err);
		throw(CfgClientException(msg));
	}
	return result;
}

static int callintapi(struct configd_conn *_conn, intapistr *api, const std::vector<std::string> &path) throw(CfgClientException)
{
	std::string cpath = mkpath(path);
	return callintapi(_conn, api, cpath);
}

static int callintapi(struct configd_conn *_conn, intapiintstr *api, int param, const std::string &str) throw(CfgClientException)
{
	struct configd_error err = { 0, };
	int result = api(_conn, param, str.c_str(), &err);
	if (result == -1) {
		std::string msg(err.text);
		configd_error_free(&err);
		throw(CfgClientException(msg));
	}
	return result;
}

static int callintapi(struct configd_conn *_conn, intapiintstr *api, int param, const std::vector<std::string> &path) throw(CfgClientException)
{
	std::string cpath = mkpath(path);
	return callintapi(_conn, api, param, cpath);
}

static std::string callstrapi(struct configd_conn *_conn, strapi *api) throw(CfgClientException)
{
	struct configd_error err = { 0, };
	char *result = api(_conn, &err);
	if (result == NULL) {
		std::string msg(err.text);
		configd_error_free(&err);
		throw(CfgClientException(msg));
	}
	std::string res = result;
	free(result);
	return res;
}

static std::string callstrapi(struct configd_conn *_conn, strapistr *api, const std::string &str) throw(CfgClientException)
{
	struct configd_error err = { 0, };
	char *result = api(_conn, str.c_str(), &err);
	if (result == NULL) {
		std::string msg(err.text);
		configd_error_free(&err);
		throw(CfgClientException(msg));
	}
	std::string res = result;
	free(result);
	return res;
}

static std::string callstrapi(struct configd_conn *_conn, strapistr *api, const std::vector<std::string> &path) throw(CfgClientException)
{
	std::string cpath = mkpath(path);
	return callstrapi(_conn, api, cpath);
}

static std::string callstrapi(struct configd_conn *_conn, strapiintstrstr *api, int p1, const std::string &str, const std::string &param) throw(CfgClientException)
{
	struct configd_error err = { 0, };
	char *result = api(_conn, p1, str.c_str(), param.c_str(), &err);
	if (result == NULL) {
		std::string msg(err.text);
		configd_error_free(&err);
		throw(CfgClientException(msg));
	}
	std::string res = result;
	free(result);
	return res;
}

static std::string callstrapi(struct configd_conn *_conn, strapiintstrstr *api, int p1, const std::vector<std::string> &path, const std::string &param) throw(CfgClientException)
{
	std::string cpath = mkpath(path);
	return callstrapi(_conn, api, p1, cpath, param);
}

static std::string callstrapi(struct configd_conn *_conn, strapiintstr *api, int param, const std::string &str) throw(CfgClientException)
{
	struct configd_error err = { 0, };
	char *result = api(_conn, param, str.c_str(), &err);
	if (result == NULL) {
		std::string msg(err.text);
		configd_error_free(&err);
		throw(CfgClientException(msg));
	}
	std::string res = result;
	free(result);
	return res;
}

static std::string callstrapi(struct configd_conn *_conn, strapiintstr *api, int param, const std::vector<std::string> &path) throw(CfgClientException)
{
	std::string cpath = mkpath(path);
	return callstrapi(_conn, api, param, cpath);
}

static std::vector<std::string> vec_to_std_vec(struct vector *vec)
{
	const char *str = NULL;
	std::vector<std::string> result;
	while ((str = vector_next(vec, str))) {
		result.push_back(str);
	}
	return result;
}

static std::vector<std::string> callvecapi(struct configd_conn *_conn, vecapistr *api, const std::vector<std::string> &path) throw(CfgClientException)
{
	struct configd_error err = { 0, };
	std::string cpath = mkpath(path);
	struct vector *vresult = api(_conn, cpath.c_str(), &err);
	if (vresult == NULL) {
		std::string msg(err.text);
		configd_error_free(&err);
		throw(CfgClientException(msg));
	}

	std::vector<std::string> result = vec_to_std_vec(vresult);
	vector_free(vresult);
	return result;
}

static std::vector<std::string> callvecapi(struct configd_conn *_conn, vecapiintstr *api, int param, const std::vector<std::string> &path) throw(CfgClientException)
{
	struct configd_error err = { 0, };
	std::string cpath = mkpath(path);
	struct vector *vresult = api(_conn, param, cpath.c_str(), &err);
	if (vresult == NULL) {
		std::string msg(err.text);
		configd_error_free(&err);
		throw(CfgClientException(msg));
	}

	std::vector<std::string> result = vec_to_std_vec(vresult);
	vector_free(vresult);
	return result;
}

static std::map<std::string, std::string> map_to_std_map(struct map *m)
{
	const char *next = NULL;
	std::map<std::string, std::string> result;
	while ((next = map_next(m, next)) != NULL) {
		std::string entry(next), key, value;
		size_t pos = entry.find("=");
		if (pos == std::string::npos) {
			continue;
		}
		key = entry.substr(0, pos);
		value = entry.substr(pos + 1, entry.length() - pos);
		result[key] = value;
	}
	return result;
}

static std::map<std::string, std::string> callmapapi(struct configd_conn *_conn, mapapi *api) throw(CfgClientException)
{
	struct configd_error err = { 0, };
	struct map *mresult = api(_conn, &err);
	if (mresult == NULL) {
		std::string msg(err.text);
		configd_error_free(&err);
		throw(CfgClientException(msg));
	}

	std::map<std::string, std::string> result = map_to_std_map(mresult);
	map_free(mresult);
	return result;
}

static std::map<std::string, std::string> callmapapi(struct configd_conn *_conn, mapapistr *api, const std::vector<std::string> &path) throw(CfgClientException)
{
	struct configd_error err = { 0, };
	std::string cpath = mkpath(path);
	struct map *mresult = api(_conn, cpath.c_str(), &err);
	if (mresult == NULL) {
		std::string msg(err.text);
		configd_error_free(&err);
		throw(CfgClientException(msg));
	}

	std::map<std::string, std::string> result = map_to_std_map(mresult);
	map_free(mresult);
	return result;
}

static std::map<std::string, std::string> callmapapi(struct configd_conn *_conn, mapapiintstr *api, int param, const std::vector<std::string> &path) throw(CfgClientException)
{
	struct configd_error err = { 0, };
	std::string cpath = mkpath(path);
	struct map *mresult = api(_conn, param, cpath.c_str(), &err);
	if (mresult == NULL) {
		std::string msg(err.text);
		configd_error_free(&err);
		throw(CfgClientException(msg));
	}

	std::map<std::string, std::string> result = map_to_std_map(mresult);
	map_free(mresult);
	return result;
}

//convert a space separated string into a vector. This can be used
//to make APIs compatible with either string paths or arrays of strings
//in the SWIG target languages
void string2vec(std::string s, std::vector<std::string> &out) {
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, ' ')) {
		out.push_back(item);
	}
}

CfgClient::CfgClient() throw(CfgClientFatalException)
{
	_conn = new struct configd_conn;
	if (configd_open_connection(_conn) < 0) {
		throw(CfgClientFatalException("failed to connect to configd"));
	}
	char *sid = getenv("VYATTA_CONFIG_SID");
	if (sid != NULL) {
		configd_set_session_id(_conn, sid);
		_sessionid = sid;
	}
}

CfgClient::~CfgClient()
{
	configd_close_connection(_conn);
	delete _conn;
}

void CfgClient::SessionAttach(const std::string &sessid) throw (CfgClientException)
{
	configd_set_session_id(_conn, sessid.c_str());
	try {
		if (SessionExists()) {
			_sessionid = sessid;
		} else {
			configd_set_session_id(_conn, _sessionid.c_str());
			throw(CfgClientException("session "+sessid+" does not exist"));
		}
	} catch (CfgClientException e) {
		configd_set_session_id(_conn, _sessionid.c_str());
		throw(e);
	}
}

void CfgClient::SessionSetup(const std::string &sessid) throw(CfgClientException)
{
	if (sessid.length() == 0) {
		throw(CfgClientException("must specify a session identifier"));
	}
	configd_set_session_id(_conn, sessid.c_str());
	callintapi(_conn, configd_sess_setup);
	_sessionid = sessid;
}

void CfgClient::SessionSetupShared(const std::string &sessid) throw(CfgClientException)
{
	if (sessid.length() == 0) {
		throw(CfgClientException("must specify a session identifier"));
	}
	configd_set_session_id(_conn, sessid.c_str());
	callintapi(_conn, configd_sess_setup_shared);
	_sessionid = sessid;
}

void CfgClient::SessionTeardown() throw(CfgClientException)
{
	callintapi(_conn, configd_sess_teardown);
}

bool CfgClient::SessionChanged() throw(CfgClientException)
{
	return callintapi(_conn, configd_sess_changed) == 1;
}

bool CfgClient::SessionSaved() throw(CfgClientException)
{
	return callintapi(_conn, configd_sess_saved) == 1;
}

void CfgClient::SessionMarkSaved() throw(CfgClientException)
{
	callintapi(_conn, configd_sess_mark_saved);
}

void CfgClient::SessionMarkUnsaved() throw(CfgClientException)
{
	callintapi(_conn, configd_sess_mark_unsaved);
}

bool CfgClient::SessionLocked() throw(CfgClientException)
{
	return callintapi(_conn, configd_sess_locked) != 0;
}

void CfgClient::SessionLock() throw(CfgClientException)
{
	callintapi(_conn, configd_sess_lock);
}

void CfgClient::SessionUnlock() throw(CfgClientException)
{
	callintapi(_conn, configd_sess_unlock);
}

bool CfgClient::SessionExists() throw(CfgClientException)
{
	return callintapi(_conn, configd_sess_exists) == 1;
}

std::string CfgClient::Set(const std::vector<std::string> &path) throw(CfgClientException)
{
	return callstrapi(_conn, configd_set, path);
}

std::string CfgClient::Delete(const std::vector<std::string> &path) throw(CfgClientException)
{
	return callstrapi(_conn, configd_delete, path);
}

std::string CfgClient::ValidatePath(const std::vector<std::string> &path) throw(CfgClientException)
{
	return callstrapi(_conn, configd_validate_path, path);
}

std::string CfgClient::Commit(const std::string &comment) throw(CfgClientException)
{
	return callstrapi(_conn, configd_commit, comment);
}

std::string CfgClient::Discard() throw(CfgClientException)
{
	return callstrapi(_conn, configd_discard);
}

std::string CfgClient::Validate() throw(CfgClientException)
{
	return callstrapi(_conn, configd_validate);
}

std::string CfgClient::ValidateConfig(const std::string encoding, const std::string config) throw(CfgClientException)
{
	struct configd_error err = {
		0,
	};

	char *result = configd_validate_config( _conn, encoding.c_str(),
			config.c_str(), &err);

	if (result == NULL)
	{
		std::string msg;
		if (err.text)
			msg = err.text;
		configd_error_free(&err);
		throw(CfgClientException(msg));
	}

	std::string res = result;
	free(result);
	return res;
}

std::string CfgClient::Save() throw(CfgClientException)
{
	struct configd_error err = { 0, };
	char *result = configd_save(_conn, NULL, &err);
	if (result == NULL) {
		std::string msg;
		if (err.text)
			msg = err.text;
		configd_error_free(&err);
		throw(CfgClientException(msg));
	}
	std::string res = result;
	free(result);
	return res;
}

bool CfgClient::Load(const std::string &file) throw(CfgClientException)
{
	return callintapi(_conn, configd_load, file) == 1;
}

bool CfgClient::Merge(const std::string &file) throw(CfgClientException)
{
	return callintapi(_conn, configd_merge, file) == 1;
}

std::map<std::string, std::string> CfgClient::TemplateGet(const std::vector<std::string> &path) throw(CfgClientException)
{
	return callmapapi(_conn, configd_tmpl_get, path);
}

std::vector<std::string> CfgClient::TemplateGetChildren(const std::vector<std::string> &path) throw(CfgClientException)
{
	return callvecapi(_conn, configd_tmpl_get_children, path);
}

std::vector<std::string> CfgClient::TemplateGetAllowed(const std::vector<std::string> &path) throw(CfgClientException)
{
	return callvecapi(_conn, configd_tmpl_get_allowed, path);
}

bool CfgClient::TemplateValidatePath(const std::vector<std::string> &path) throw(CfgClientException)
{
	return callintapi(_conn, configd_tmpl_validate_path, path);
}

bool CfgClient::TemplateValidateValues(const std::vector<std::string> &path) throw(CfgClientException)
{
	return callintapi(_conn, configd_tmpl_validate_values, path);
}

static void split(const std::string &s, char delim, std::vector<std::string> &v)
{
	size_t pos = s.find(delim);
	if (pos == std::string::npos) {
		// No delimiter, just put the one item in the list
		if (s.length() != 0) {
			v.push_back(s);
		}
		return;
	}
	for (size_t i = 0; pos != std::string::npos; ) {
		v.push_back(s.substr(i, pos-i));
		i = ++pos;
		pos = s.find(delim, pos);

		if (pos == std::string::npos) {
			v.push_back(s.substr(i, s.length()));
		}
	}
}

std::map<std::string, std::vector<std::string> > CfgClient::GetFeatures(void) throw(CfgClientException)
{
	std::map<std::string, std::vector<std::string> > features;
	std::map<std::string, std::string> m = callmapapi(_conn, configd_get_features);
	std::map<std::string, std::string>::iterator it = m.begin();
	for (; it != m.end(); ++it) {
		std::vector<std::string> v;
		split((*it).second, ',', v);
		features.insert(std::pair<std::string,std::vector<std::string> >((*it).first, v));
	}
	return features;
}

std::map<std::string, std::string> CfgClient::GetHelp(const std::vector<std::string> &path, bool fromSchema) throw(CfgClientException)
{
	return callmapapi(_conn, configd_get_help, fromSchema, path);
}

bool CfgClient::NodeExists(Database db, const std::vector<std::string> &path) throw(CfgClientException)
{
	return callintapi(_conn, configd_node_exists, db, path);
}

bool CfgClient::NodeIsDefault(Database db, const std::vector<std::string> &path) throw(CfgClientException)
{
	return callintapi(_conn, configd_node_is_default, db, path);
}

std::vector<std::string> CfgClient::NodeGet(Database db, const std::vector<std::string> &path) throw(CfgClientException)
{
	return callvecapi(_conn, configd_node_get, db, path);
}

CfgClient::NodeStatus CfgClient::NodeGetStatus(Database db, const std::vector<std::string> &path) throw(CfgClientException)
{
	return static_cast<CfgClient::NodeStatus>(callintapi(_conn, configd_node_get_status, db, path));
}

CfgClient::NodeType CfgClient::NodeGetType(const std::vector<std::string> &path) throw(CfgClientException)
{
	return static_cast<CfgClient::NodeType>(callintapi(_conn, configd_node_get_type, path));
}

std::string CfgClient::TreeGetEncoding(Database db, const std::vector<std::string> &path, const std::string &encoding) throw(CfgClientException)
{
	return callstrapi(_conn, configd_tree_get_encoding, db, path, encoding);
}

std::string CfgClient::TreeGetFullEncoding(Database db, const std::vector<std::string> &path, const std::string &encoding) throw(CfgClientException)
{
	return callstrapi(_conn, configd_tree_get_full_encoding, db, path, encoding);
}

std::string CfgClient::TreeGet(Database db, const std::vector<std::string> &path) throw(CfgClientException)
{
	return callstrapi(_conn, configd_tree_get, db, path);
}

std::string CfgClient::TreeGetXML(Database db, const std::vector<std::string> &path) throw(CfgClientException)
{
	return callstrapi(_conn, configd_tree_get_xml, db, path);
}

std::string CfgClient::TreeGetInternal(Database db, const std::vector<std::string> &path) throw(CfgClientException)
{
	return callstrapi(_conn, configd_tree_get_internal, db, path);
}

std::string CfgClient::TreeGetFull(Database db, const std::vector<std::string> &path) throw(CfgClientException)
{
	return callstrapi(_conn, configd_tree_get_full, db, path);
}

std::string CfgClient::TreeGetFullXML(Database db, const std::vector<std::string> &path) throw(CfgClientException)
{
	return callstrapi(_conn, configd_tree_get_full_xml, db, path);
}

std::string CfgClient::TreeGetFullInternal(Database db, const std::vector<std::string> &path) throw(CfgClientException)
{
	return callstrapi(_conn, configd_tree_get_full_internal, db, path);
}

std::string CfgClient::CallRPC(const std::string ns, const std::string name, const std::string input) throw(CfgClientException)
{
	struct configd_error err = { 0, };
	char *result = configd_call_rpc(_conn, ns.c_str(), name.c_str(), input.c_str(), &err);
	if (result == NULL) {
		std::string msg;
		if (err.text)
			msg = err.text;
		configd_error_free(&err);
		throw(CfgClientException(msg));
	}
	std::string res = result;
	free(result);
	return res;
}

std::string CfgClient::CallRPCXML(const std::string ns, const std::string name, const std::string input) throw(CfgClientException)
{
	struct configd_error err = { 0, };
	char *result = configd_call_rpc_xml(_conn, ns.c_str(), name.c_str(), input.c_str(), &err);
	if (result == NULL) {
		std::string msg;
		if (err.text)
			msg = err.text;
		configd_error_free(&err);
		throw(CfgClientException(msg));
	}
	std::string res = result;
	free(result);
	return res;
}

std::string CfgClient::EditConfigXML(const std::string target, const std::string default_operation, const std::string test_option, const std::string error_option, const std::string config) throw(CfgClientException) {
	struct configd_error err = { 0, };
	char *result = configd_edit_config_xml(_conn, target.c_str(), default_operation.c_str(),
		test_option.c_str(), error_option.c_str(), config.c_str(), &err);
	if (result == NULL) {
		std::string msg;
		if (err.text)
			msg = err.text;
		configd_error_free(&err);
		throw(CfgClientException(msg));
	}
	std::string res = result;
	free(result);
	return res;
}

std::string CfgClient::CopyConfig(
	const std::string source_datastore,
	const std::string source_encoding,
	const std::string source_config,
	const std::string source_url,
	const std::string target_datastore,
	const std::string target_url) throw(CfgClientException)
{
	struct configd_error err = {
		0,
	};

	char *result = configd_copy_config(_conn, source_datastore.c_str(),
		source_encoding.c_str(), source_config.c_str(),
		source_url.c_str(), target_datastore.c_str(), target_url.c_str(),
		&err);

	if (result == NULL)
	{
		std::string msg;
		if (err.text)
			msg = err.text;
		configd_error_free(&err);
		throw(CfgClientException(msg));
	}

	std::string res = result;
	free(result);
	return res;
}

// Private functions
