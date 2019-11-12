/*
 * Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
 *
 * Copyright (c) 2014-2015 by Brocade Communications Systems, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 */
#include <map>
#include <string>
#include <vector>

#include <rpc.h>

#include "connect.h"

typedef std::vector<std::string> StringVector;
typedef std::map<std::string,std::string> StringMap;

namespace cstore {
class Cstore {
public:
	Cstore();
	~Cstore();

	void connect();
	void disconnect();
	bool cfgGetTree(StringVector&, std::string&, bool);
	bool cfgPathGetType(StringVector &path, std::string &result);
	bool cfgPathExists(StringVector &path, bool active_cfg);
	bool cfgPathEffective(StringVector &path);
	bool cfgPathDefault(StringVector &path, bool active_cfg);

	void cfgPathGetChildNodes(StringVector &path, StringVector &result, bool active_cfg);
	bool cfgPathGetValue(StringVector &path, std::string &result, bool active_cfg);
	bool cfgPathGetValues(StringVector &path, StringVector &result, bool active_cfg);
	bool cfgPathGetComment(StringVector &path, std::string &result, bool active_cfg);

	void cfgPathGetEffectiveChildNodes(StringVector &path, StringVector &result);
	bool cfgPathGetEffectiveValue(StringVector &path, std::string &result);
	bool cfgPathGetEffectiveValues(StringVector &path, StringVector &result);

	bool cfgPathDeleted(StringVector &path);
	bool cfgPathAdded(StringVector &path);
	bool cfgPathChanged(StringVector &path);
	bool cfgPathStatus(StringVector &path, std::string &result);
	void cfgPathGetChildNodesStatus(StringVector &path, StringMap &result);

	void cfgPathGetDeletedChildNodes(StringVector &path, StringVector &result);
	void cfgPathGetDeletedValues(StringVector &path, StringVector &result);

	bool getParsedTmpl(StringVector &path, StringMap &result, bool allow_val);
	void tmplGetChildNodes(StringVector &path, StringVector &result);
	bool validateTmplPath(StringVector &path, bool validate_vals);

	bool sessionChanged();
	bool inSession();
	bool loadFile(char *filename);

private:
	void assertSession(const char *func);
	void vecToStrVec(struct vector *v, StringVector &sv);
	char *strVecToChar(StringVector &sv);
	bool nodeIsStatus(StringVector &path, NodeStatus stat);
	bool nodeIsChanged(StringVector &path);
	bool nodeGetDB(int Db, StringVector &args, StringVector &result);
	bool nodeGet(StringVector &args, StringVector &result, bool active, bool effective);
	bool nodeExistsDB(int Db, StringVector &path);
	bool nodeExists(StringVector &path, bool active, bool effective);
	void exit_err(const char *fmt, ...);
	void vexit_err(const char *fmt, va_list alist);

	static bool conn_established;
	static struct configd_conn *conn;
};

}
