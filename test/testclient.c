/*
 * Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
 *
   Copyright (c) 2013-2015 by Brocade Communications Systems, Inc.
   All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
*/

#include <argz.h>
#include <errno.h>
#include <fcntl.h>
#include <jansson.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include <vyatta-util/map.h>
#include <vyatta-util/paths.h>
#include <vyatta-util/vector.h>

#include "auth.h"
#include "connect.h"
#include "error.h"
#include "log.h"
#include "node.h"
#include "rpc.h"
#include "session.h"
#include "template.h"
#include "transaction.h"

/*

  To use this test utility, first run the following commands on the
  DUT:

  sudo mkdir /var/run/configd
  sudo /opt/vyatta/sbin/configd

 */

static int debug;

enum {
	SESSION_EXISTS,
	SESSION_SETUP,
	SESSION_TEARDOWN,
	SESSION_CHANGED,
	SESSION_SAVED,
	SESSION_MARK_SAVED,	//5
	SESSION_MARK_UNSAVED,
	SESSION_GET_ENV,
	SESSION_LOCK,
	SESSION_UNLOCK,
	SESSION_LOCKED,		//10
	TMPL_GET,
	TMPL_GET_CHILDREN,
	TMPL_GET_ALLOWED,
	TMPL_VALIDATE_PATH,
	TMPL_VALIDATE_VALUES,	//15
	EDIT_GET_ENV,
	NODE_EXISTS,
	NODE_IS_DEFAULT,
	NODE_GET,
	NODE_GET_STATUS,	//20
	NODE_GET_TYPE,
	NODE_GET_COMPLETE_ENV,
	NODE_GET_COMMENT,
	SHOW,
	SET,			//25
	DELETE,
	RENAME,
	COPY,
	COMMENT,
	COMMIT,			//30
	DISCARD,
	SAVE,
	LOAD,
	VALIDATE,
	VALIDATE_PATH,		//35
	AUTH_AUTHORIZE,
	AUTH_GETPERMS,
	TREE_GET,
	TREE_GET_JSON_FLAGS,
	TREE_GET_XML_FLAGS,	//40
	SCHEMA_GET,
	GET_SCHEMAS,
	GET_FEATURES,
	GET_HELP,
	STRESS,			//45
	CMD_MAX
};

static const char *cmd_table[] = {
	[SESSION_EXISTS] = "SessionExists",
	[SESSION_SETUP] = "SessionSetup",
	[SESSION_TEARDOWN] = "SessionTeardown",
	[SESSION_CHANGED] = "SessionChanged",
	[SESSION_SAVED] = "SessionSaved",
	[SESSION_MARK_SAVED] = "SessionMarkSaved",
	[SESSION_MARK_UNSAVED] = "SessionMarkUnsaved",
	[SESSION_GET_ENV] = "SessionGetEnv",
	[SESSION_LOCK] = "SessionLock",
	[SESSION_UNLOCK] = "SessionUnlock",
	[SESSION_LOCKED] = "SessionLocked",
	[EDIT_GET_ENV] = "EditGetEnv",
	[NODE_EXISTS] = "NodeExists",
	[NODE_IS_DEFAULT] = "NodeIsDefault",
	[NODE_GET] = "NodeGet",
	[NODE_GET_STATUS] = "NodeGetStatus",
	[NODE_GET_TYPE] = "NodeGetType",
	[NODE_GET_COMPLETE_ENV] = "NodeGetCompleteEnv",
	[NODE_GET_COMMENT] = "NodeGetComment",
	[TMPL_GET] = "TmplGet",
	[TMPL_GET_CHILDREN] = "TmplGetChildren",
	[TMPL_GET_ALLOWED] = "TmplGetAllowed",
	[TMPL_VALIDATE_PATH] = "TmplValidatePath",
	[TMPL_VALIDATE_VALUES] = "TmplValidateValues",
	[SHOW] = "Show",
	[SET] = "Set",
	[DELETE] = "Delete",
	[RENAME] = "Rename",
	[COPY] = "Copy",
	[COMMENT] = "Comment",
	[COMMIT] = "Commit",
	[DISCARD] = "Discard",
	[SAVE] = "Save",
	[LOAD] = "Load",
	[VALIDATE] = "Validate",
	[VALIDATE_PATH] = "ValidatePath",
	[AUTH_GETPERMS] = "GetPerms",
	[AUTH_AUTHORIZE] = "Authorize",
	[TREE_GET] = "TreeGet",
	[TREE_GET_JSON_FLAGS] = "TreeGetJsonFlags",
	[TREE_GET_XML_FLAGS] = "TreeGetXmlFlags",
	[SCHEMA_GET] = "SchemaGet",
	[GET_SCHEMAS] = "GetSchemas",
	[GET_FEATURES] = "GetFeatures",
	[GET_HELP] = "GetHelp",
	[STRESS] = "Stress",
	[CMD_MAX] = NULL
};

static const char SID_ENV[] = "VYATTA_CONFIG_SID";

static const char *db_name[] = {
	[AUTO] = "Auto",
	[RUNNING] = "Running",
	[CANDIDATE] = "Candidate",
	[EFFECTIVE] = "Effective",
};


static const char *get_db_name(int db)
{
	if (db < RUNNING || db > EFFECTIVE)
		return "Unknown";
	return db_name[db];
}

static int map_db_name(const char *db)
{
	for (int i = 0; i <= EFFECTIVE; ++i)
		if (strcasecmp(db, db_name[i]) == 0)
			return i;
	return -1;
}

static const char *get_node_status_str(int status)
{
	static const char *node_status[] = {
		[NODE_STATUS_UNCHANGED] = "Unchanged",
		[NODE_STATUS_CHANGED] = "Changed",
		[NODE_STATUS_ADDED] = "Added",
		[NODE_STATUS_DELETED] = "Deleted",
	};
	if (status < NODE_STATUS_UNCHANGED || status > NODE_STATUS_DELETED)
		return "Unknown";
	return node_status[status];
}

static const char *get_node_type_str(int type)
{
	static const char *node_type[] = {
		[NODE_TYPE_LEAF] = "Leaf",
		[NODE_TYPE_MULTI] = "Multi-leaf",
		[NODE_TYPE_CONTAINER] = "Container",
		[NODE_TYPE_TAG] = "Tag",
	};
	if (type < NODE_TYPE_LEAF || type > NODE_TYPE_TAG)
		return "Unknown";
	return node_type[type];
}

static void dump_error(struct configd_error *err)
{
	if (err->text) {
		fprintf(stderr, "\tMessage: %s", err->text);
		if (err->source)
			fprintf(stderr, "\tSource: %s", err->source);
		fputs("\n", stderr);
	}
	configd_error_free(err);
}

static int dispatch(struct configd_conn *conn, int cmd_id, char *arg, size_t arg_size, int db)
{
	int result;
	char *buf;
	struct vector *v;
	struct map *m;
	char *path;
	const char *str = NULL;
	struct configd_error err = {0};

	switch (cmd_id) {
	case SESSION_EXISTS:
		switch (configd_sess_exists(conn, &err)) {
		case 0:
			fprintf(stdout, "Session %s does not exist\n",
				conn->session_id);
			break;
		case 1:
			fprintf(stdout, "Session %s exists\n",
				conn->session_id);
			break;
		default:
			fprintf(stdout, "ERROR checking session exists\n");
			dump_error(&err);
			return -1;
		}
		break;
	case SESSION_CHANGED:
		switch (configd_sess_changed(conn, &err)) {
		case 0:
			fprintf(stdout, "Session %s not changed\n", conn->session_id);
			break;
		case 1:
			fprintf(stdout, "Session %s is changed\n", conn->session_id);
			break;
		default:
			fprintf(stdout, "ERROR checking session changed\n");
			dump_error(&err);
			return -1;
		}
		break;
	case SESSION_SAVED:
		switch (configd_sess_saved(conn, &err)) {
		case 0:
			fprintf(stdout, "Session %s not saved\n", conn->session_id);
			break;
		case 1:
			fprintf(stdout, "Session %s is saved\n", conn->session_id);
			break;
		default:
			fprintf(stdout, "ERROR checking session saved\n");
			dump_error(&err);
			return -1;
		}
		break;
	case SESSION_MARK_SAVED:
		if (configd_sess_mark_saved(conn, &err) != 0) {
			fprintf(stdout, "ERROR marking session saved\n");
			dump_error(&err);
			return -1;
		}
		fprintf(stdout, "Session %s marked saved\n", conn->session_id);
		break;
	case SESSION_MARK_UNSAVED:
		if (configd_sess_mark_unsaved(conn, &err) != 0) {
			fprintf(stdout, "ERROR marking session unsaved\n");
			dump_error(&err);
			return -1;
		}
		fprintf(stdout, "Session %s marked unsaved\n", conn->session_id);
		break;
	case SESSION_GET_ENV:
		buf = configd_sess_get_env(conn, &err);
		if (!buf) {
			fprintf(stdout, "ERROR getting environment variables\n");
			dump_error(&err);
			return -1;
		}

		fprintf(stdout, "Environment variables:\n%s\n", buf);
		free(buf);
		break;
	case SESSION_LOCK:
		if (configd_sess_lock(conn, &err) != 0) {
			fprintf(stdout, "ERROR locking session\n");
			dump_error(&err);
			return -1;
		}
		fprintf(stdout, "Session %s locked\n", conn->session_id);
		break;
	case SESSION_UNLOCK:
		if (configd_sess_unlock(conn, &err) != 0) {
			fprintf(stdout, "ERROR unlocking session\n");
			dump_error(&err);
			return -1;
		}
		fprintf(stdout, "Session %s unlocked\n", conn->session_id);
		break;
	case SESSION_LOCKED:
		switch (configd_sess_locked(conn, &err)) {
		case 0:
			fprintf(stdout, "Session %s not locked\n", conn->session_id);
			break;
		case 1:
			fprintf(stdout, "Session %s is locked\n", conn->session_id);
			break;
		default:
			fprintf(stdout, "ERROR checking session lock\n");
			dump_error(&err);
			return -1;
		}
		break;
	case EDIT_GET_ENV:
		path = args_to_path(arg, arg_size);
		if (!path)
			return -1;
		buf = configd_edit_get_env(conn, path, &err);
		if (!buf) {
			fprintf(stdout, "ERROR getting environment variables\n");
			dump_error(&err);
			return -1;
		}

		fprintf(stdout, "Environment variables:\n%s\n", buf);
		free(buf);
		free(path);
		break;
	case NODE_GET_COMPLETE_ENV:
		// E.g., delete ""
		// E.g., set interfaces ""
		path = args_to_path(arg, arg_size);
		if (!path)
			return -1;
		buf = configd_node_get_complete_env(conn, path, &err);
		if (!buf) {
			fprintf(stdout, "ERROR getting completion environment variables\n");
			dump_error(&err);
			return -1;
		}

		fprintf(stdout, "Completion environment variables:\n%s\n", buf);
		free(buf);
		free(path);
		break;
	case NODE_EXISTS:
		path = args_to_path(arg, arg_size);
		if (!path)
			return -1;
		argz_stringify(arg, arg_size, ' ');
		switch (configd_node_exists(conn, db, path, &err)) {
		case 0:
			fprintf(stdout, "Node [%s] does not exist in %s db\n",
				arg, get_db_name(db));
			break;
		case 1:
			fprintf(stdout, "Node [%s] exists in %s db\n",
				arg, get_db_name(db));
			break;
		default:
			fprintf(stdout, "ERROR checking node exists\n");
			dump_error(&err);
			free(path);
			return -1;
		}
		free(path);
		break;
	case NODE_IS_DEFAULT:
		path = args_to_path(arg, arg_size);
		if (!path)
			return -1;
		argz_stringify(arg, arg_size, ' ');
		switch (configd_node_is_default(conn, db, path, &err)) {
		case 0:
			fprintf(stdout, "Node [%s] is not default in %s db\n",
				arg, get_db_name(db));
			break;
		case 1:
			fprintf(stdout, "Node [%s] is default in %s db\n",
				arg, get_db_name(db));
			break;
		default:
			fprintf(stdout, "ERROR checking if node is default\n");
			dump_error(&err);
			free(path);
			return -1;
		}
		free(path);
		break;
	case NODE_GET:
		path = args_to_path(arg, arg_size);
		if (!path)
			return -1;
		v = configd_node_get(conn, db, path, &err);
		if (!v) {
			fprintf(stdout, "ERROR getting node value\n");
			dump_error(&err);
			free(path);
			return -1;
		}
		argz_stringify(arg, arg_size, ' ');
		fprintf(stdout, "Node [%s] is:\n", arg);
		free(path);
		while ((str = vector_next(v, str)))
			fprintf(stdout, "\t%s\n", str);
		vector_free(v);
		break;
	case NODE_GET_STATUS:
		path = args_to_path(arg, arg_size);
		if (!path)
			return -1;
		result = configd_node_get_status(conn, db, path, &err);
		if (result < 0) {
			fprintf(stdout, "ERROR getting node status\n");
			dump_error(&err);
			free(path);
			return -1;
		}
		argz_stringify(arg, arg_size, ' ');
		fprintf(stdout, "Node [%s] has status %s\n", arg, get_node_status_str(result));
		free(path);
		break;
	case NODE_GET_TYPE:
		path = args_to_path(arg, arg_size);
		if (!path)
			return -1;
		argz_stringify(arg, arg_size, ' ');
		result = configd_node_get_type(conn, path, &err);
		if (result < 0) {
			fprintf(stdout, "ERROR getting node type for [%s]\n", arg);
			dump_error(&err);
			free(path);
			return -1;
		}
		fprintf(stdout, "Node [%s] is %s type\n", arg, get_node_type_str(result));
		free(path);
		break;
	case NODE_GET_COMMENT:
		path = args_to_path(arg, arg_size);
		if (!path)
			return -1;
		buf = configd_node_get_comment(conn, db, path, &err);
		if (!buf) {
			fprintf(stdout, "ERROR getting comment\n");
			dump_error(&err);
			free(path);
			return -1;
		}
		argz_stringify(arg, arg_size, ' ');
		fprintf(stdout, "Comment for [%s]:\n%s\n", arg, buf);
		free(buf);
		free(path);
		break;
	case TREE_GET:
		path = args_to_path(arg, arg_size);
		if (!path)
			return -1;
		buf = configd_tree_get(conn, db, path, &err);
		if (!buf) {
			fprintf(stdout, "ERROR getting tree\n");
			dump_error(&err);
			free(path);
			return -1;
		}

		fprintf(stdout, "Tree:\n%s\n", buf);
		free(buf);
		free(path);
		break;
	case TREE_GET_JSON_FLAGS:
		#define TREE_GET_JSON_PATH "/system/login"
		path = args_to_path(TREE_GET_JSON_PATH, strlen(TREE_GET_JSON_PATH));
		if (!path)
			return -1;
		result = atoi(arg);
		buf = configd_tree_get_encoding_flags(conn, db,
						      TREE_GET_JSON_PATH, "json",
						      result, &err);
		if (!buf) {
			fprintf(stdout, "ERROR getting tree\n");
			dump_error(&err);
			return -1;
		}

		fprintf(stdout, "Tree:\n%s\n", buf);
		free(buf);
		break;
	case TREE_GET_XML_FLAGS:
		#define TREE_GET_XML_PATH "/system/login"
		path = args_to_path(TREE_GET_XML_PATH, strlen(TREE_GET_XML_PATH));
		if (!path)
			return -1;
		result = atoi(arg);
		buf = configd_tree_get_encoding_flags(conn, db,
						      TREE_GET_XML_PATH, "xml",
						      result, &err);
		if (!buf) {
			fprintf(stdout, "ERROR getting tree\n");
			dump_error(&err);
			return -1;
		}

		fprintf(stdout, "Tree:\n%s\n", buf);
		free(buf);
		break;
	case TMPL_VALIDATE_PATH:
		path = args_to_path(arg, arg_size);
		if (!path)
			return -1;
		argz_stringify(arg, arg_size, ' ');
		switch (configd_tmpl_validate_path(conn, path, &err)) {
		case 0:
			fprintf(stdout, "Template %s does not exist\n", arg);
			break;
		case 1:
			fprintf(stdout, "Template %s exists\n", arg);
			break;
		default:
			fprintf(stdout, "ERROR validating template path\n");
			dump_error(&err);
			free(path);
			return -1;
		}
		free(path);
		break;
	case TMPL_VALIDATE_VALUES:
		path = args_to_path(arg, arg_size);
		if (!path)
			return -1;
		argz_stringify(arg, arg_size, ' ');
		switch (configd_tmpl_validate_values(conn, path, &err)) {
		case 0:
			fprintf(stdout, "Template value %s does not exist\n", arg);
			break;
		case 1:
			fprintf(stdout, "Template value %s exists\n", arg);
			break;
		default:
			fprintf(stdout, "ERROR validating template values\n");
			dump_error(&err);
			free(path);
			return -1;
		}
		free(path);
		break;
	case TMPL_GET_CHILDREN:
		path = args_to_path(arg, arg_size);
		if (!path)
			return -1;
		v = configd_tmpl_get_children(conn, path, &err);
		if (!v) {
			fprintf(stdout, "ERROR getting template\n");
			dump_error(&err);
			free(path);
			return -1;
		}
		argz_stringify(arg, arg_size, ' ');
		fprintf(stdout, "Template '%s' has children:\n", arg);
		free(path);
		while ((str = vector_next(v, str)))
			fprintf(stdout, "\t%s\n", str);
		vector_free(v);
		break;
	case TMPL_GET_ALLOWED:
		path = args_to_path(arg, arg_size);
		if (!path)
			return -1;
		v = configd_tmpl_get_allowed(conn, path, &err);
		if (!v) {
			fprintf(stdout, "ERROR getting template\n");
			dump_error(&err);
			free(path);
			return -1;
		}
		argz_stringify(arg, arg_size, ' ');
		fprintf(stdout, "Template '%s' has children:\n", arg);
		free(path);
		while ((str = vector_next(v, str)))
			fprintf(stdout, "\t%s\n", str);
		vector_free(v);
		break;

	case TMPL_GET:
		path = args_to_path(arg, arg_size);
		if (!path)
			return -1;
		m = configd_tmpl_get(conn, path, &err);
		if (!m) {
			fprintf(stdout, "ERROR getting template\n");
			dump_error(&err);
			free(path);
			return -1;
		}
		argz_stringify(arg, arg_size, ' ');
		fprintf(stdout, "Template '%s' is:\n", arg);
		free(path);
		while ((str = map_next(m, str)))
			fprintf(stdout, "\t%s\n", str);
		map_free(m);
		break;
	case SHOW:
		path = args_to_path(arg, arg_size);
		if (!path)
			return -1;
		buf = configd_show(conn, db, path, "@ACTIVE", "@WORKING", SHOWF_COMMANDS, &err);
		if (!buf) {
			fprintf(stdout, "ERROR performing show\n");
			dump_error(&err);
			free(path);
			return -1;
		}
		argz_stringify(arg, arg_size, ' ');
		fprintf(stdout, "Show %s:\n%s\n", arg, buf);
		free(buf);
		free(path);
		break;
	case SET:
		path = args_to_path(arg, arg_size);
		if (!path)
			return -1;
		buf = configd_set(conn, path, &err);
		if (!buf) {
			fprintf(stdout, "ERROR performing set\n");
			dump_error(&err);
			free(path);
			return -1;
		}
		argz_stringify(arg, arg_size, ' ');
		fprintf(stdout, "Set %s:\n\t%s\n", arg, buf);
		free(buf);
		free(path);
		break;
	case DELETE:
		path = args_to_path(arg, arg_size);
		if (!path)
			return -1;
		buf = configd_delete(conn, path, &err);
		if (!buf) {
			fprintf(stdout, "ERROR performing delete\n");
			dump_error(&err);
			free(path);
			return -1;
		}
		argz_stringify(arg, arg_size, ' ');
		fprintf(stdout, "Deleted %s:\n\t%s\n", arg, buf);
		free(buf);
		free(path);
		break;
	case RENAME:
		// E.g., /interfaces/ethernet/eth2/vif/100/to/200
		path = args_to_path(arg, arg_size);
		if (!path)
			return -1;
		printf("Rename: %s\n", path);
		buf = configd_rename(conn, path, &err);
		free(path);
		if (!buf) {
			fprintf(stdout, "ERROR performing rename\n");
			dump_error(&err);
			return -1;
		}
		argz_stringify(arg, arg_size, ' ');
		fprintf(stdout, "Renamed %s:\n\t%s\n", arg, buf);
		free(buf);
		break;
	case COPY:
		// E.g., /interfaces/ethernet/eth2/vif/100/to/200
		path = args_to_path(arg, arg_size);
		if (!path)
			return -1;
		buf = configd_copy(conn, path, &err);
		free(path);
		if (!buf) {
			fprintf(stdout, "ERROR performing copy\n");
			dump_error(&err);
			return -1;
		}
		argz_stringify(arg, arg_size, ' ');
		fprintf(stdout, "Copied %s:\n\t%s\n", arg, buf);
		free(buf);
		break;
	case COMMENT:
		path = args_to_path(arg, arg_size);
		if (!path)
			return -1;
		buf = configd_comment(conn, path, &err);
		if (!buf) {
			fprintf(stdout, "ERROR performing comment\n");
			dump_error(&err);
			free(path);
			return -1;
		}
		argz_stringify(arg, arg_size, ' ');
		fprintf(stdout, "Commented %s:\n\t%s\n", arg, buf);
		free(buf);
		free(path);
		break;
	case COMMIT:
		buf = configd_commit(conn, arg, &err);
		if (!buf) {
			fprintf(stdout, "ERROR performing commit\n");
			dump_error(&err);
			return -1;
		}
		fprintf(stdout, "Committed\n%s\n", buf);
		free(buf);
		break;
	case DISCARD:
		buf = configd_discard(conn, &err);
		if (!buf) {
			fprintf(stdout, "ERROR performing discard\n");
			dump_error(&err);
			return -1;
		}
		fprintf(stdout, "Discarded\n%s\n", buf);
		free(buf);
		break;
	case SAVE:
		path = args_to_path(arg, arg_size);
		if (!path)
			return -1;
		buf = configd_save(conn, path, &err);
		if (!buf) {
			fprintf(stdout, "ERROR performing save\n");
			dump_error(&err);
			free(path);
			return -1;
		}
		fprintf(stdout, "Saved\n%s\n", buf);
		free(buf);
		free(path);
		break;
	case LOAD:
		path = args_to_path(arg, arg_size);
		if (!path)
			return -1;
		argz_stringify(arg, arg_size, ' ');
		switch (configd_load(conn, path, &err)) {
		case 0:
			fprintf(stdout, "Unable to load config from %s\n", arg);
			break;
		case 1:
			fprintf(stdout, "Loaded config from %s\n", arg);
			break;
		default:
			fprintf(stdout, "ERROR loading config\n");
			dump_error(&err);
			free(path);
			return -1;
		}
		free(path);
		break;
	case VALIDATE:
		buf = configd_validate(conn, &err);
		if (!buf) {
			fprintf(stdout, "ERROR validating\n");
			dump_error(&err);
			return -1;
		}
		fprintf(stdout, "Validated:\n\t%s\n", buf);
		free(buf);
		break;
	case VALIDATE_PATH:
		path = args_to_path(arg, arg_size);
		if (!path)
			return -1;
		buf = configd_validate_path(conn, path, &err);
		free(path);
		if (!buf) {
			fprintf(stdout, "ERROR validating\n");
			dump_error(&err);
			return -1;
		}
		fprintf(stdout, "Validated:\n\t%s\n", buf);
		free(buf);
		break;
	case STRESS:
		for (int i = 0; i < 10; i++) {
			printf("%d\n", i);
			for (int j = 0; j < 10000; j++) {
				char *buf1;
				struct configd_error err;
				if (asprintf(&buf1, "/foo/%d/bar/%d/baz/%d %d", i, j, i, j) == -1) {
					free(buf1);
					return -1;
				}
				buf = configd_set(conn, buf1, &err);
				if (!buf) {
					fprintf(stdout, "ERROR performing set\n");
					dump_error(&err);
					continue;
				}
				//fprintf(stdout, "Set %s:\n\t%s\n", buf1, buf);
				free(buf);
				free(buf1);
			}
		}
		break;
	case AUTH_GETPERMS:
		m = configd_auth_getperms(conn, &err);
		if (!m) {
			fprintf(stdout, "ERROR getting permissions\n");
			dump_error(&err);
			return -1;
		}
		fprintf(stdout, "permissions are:\n");
		while ((str = map_next(m, str)))
			fprintf(stdout, "\t%s\n", str);
		map_free(m);
		break;
	case AUTH_AUTHORIZE:
		path = args_to_path(arg, arg_size);
		if (!path)
			return -1;
		argz_stringify(arg, arg_size, ' ');
		switch (configd_auth_authorized(conn, path, 2, &err)) {
		case 0:
			fprintf(stdout, "Read Access Denied %s\n", arg);
			break;
		case 1:
			fprintf(stdout, "Read Access Granted %s\n", arg);
			break;
		default:
			fprintf(stdout, "ERROR authorizing path\n");
			dump_error(&err);
			free(path);
			return -1;
		}
		free(path);
		break;
	case SCHEMA_GET:
		buf = configd_schema_get(conn, arg, "", &err);
		if (!buf) {
			fprintf(stdout, "ERROR getting tree\n");
			dump_error(&err);
			return -1;
		}

		fprintf(stdout, "Tree:\n%s\n", buf);
		free(buf);
		break;
	case GET_SCHEMAS:
		buf = configd_get_schemas(conn, &err);
		if (!buf) {
			fprintf(stdout, "ERROR getting schema list\n");
			dump_error(&err);
			return -1;
		}

		fprintf(stdout, "Schemas:\n%s\n", buf);
		free(buf);
		break;
	case GET_FEATURES:
		m = configd_get_features(conn, &err);
		if (!m) {
			fprintf(stdout, "ERROR getting schema feature list\n");
			dump_error(&err);
			return -1;
		}

		fprintf(stdout, "Features:\n");
		while ((str = map_next(m, str)))
			fprintf(stdout, "\t%s\n", str);
		map_free(m);
		break;
	case GET_HELP:
		path = args_to_path(arg, arg_size);
		if (!path)
			return -1;
		m = configd_get_help(conn, 1, path,  &err);
		if (!m) {
			fprintf(stdout, "ERROR getting help\n");
			dump_error(&err);
			return -1;
		}
		fprintf(stdout, "help is:\n");
		while ((str = map_next(m, str)))
			fprintf(stdout, "\t%s\n", str);
		map_free(m);
		break;
	default:
		fprintf(stdout, "Unknown command %d\n", cmd_id);
		return -1;
	}
	return 0;
}


int main (int argc, char **argv) {
	int init = 0;
	int end = 0;
	int lock = 0;
	int unlock = 0;
	int result = EXIT_SUCCESS;
	struct configd_conn conn;
	int opt;
	const char *sid = NULL;
	const char *cmd = NULL;
	char *args = NULL;
	size_t args_size = 0;
	int db = AUTO;
	struct configd_error err = {0};

	while ((opt = getopt(argc, argv, ":b:deils:u")) != -1) {
		switch (opt) {
		case 'b':
			db = map_db_name(optarg);
			if (db == -1) {
				fprintf(stderr, "Invalid DB name %s\n", optarg);
				exit(EXIT_FAILURE);
			}
			break;
		case 'd':
			++debug;
			break;
		case 'e':
			++end;
			break;
		case 'i':
			++init;
			break;
		case 'l':
			++lock;
			break;
		case 's':
			sid = optarg;
			break;
		case 'u':
			++unlock;
			break;
		case ':':
			fprintf(stderr, "Option %c is missing a parameter; ignoring\n", optopt);
			break;
		case '?':
		default:
			fprintf(stderr, "Unknown option %c; ignoring\n", optopt);
			break;
		}
	}

	if (optind < argc)
		cmd = argv[optind++];
	while (optind < argc)
		if (argz_add(&args, &args_size, argv[optind++])) {
			fprintf(stderr, "Unable to allocate args\n");
			exit(EXIT_FAILURE);
		}

	msg_use_console(debug);

	if (configd_open_connection(&conn) == -1) {
		fprintf(stderr, "Unable to open connection: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (!sid)
		sid = getenv(SID_ENV);
	if (sid)
		configd_set_session_id(&conn, sid);

	if (init) {
		fprintf(stdout, "Creating session with id %s\n", conn.session_id);
		if (configd_sess_setup(&conn, &err) == -1) {
			fprintf(stderr, "Unable to create session\n");
			dump_error(&err);
			result = EXIT_FAILURE;
			goto error;
		}
		fprintf(stdout, "Created session %s\n", conn.session_id);
	}

	if (lock) {
		if (configd_sess_lock(&conn, &err) == -1) {
			fprintf(stderr, "Unable to lock session\n");
			dump_error(&err);
			result = EXIT_FAILURE;
			goto error;
		}
	}

	if (unlock) {
		if (configd_sess_unlock(&conn, &err) == -1) {
			fprintf(stderr, "Unable to lock session\n");
			dump_error(&err);
			result = EXIT_FAILURE;
			goto error;
		}
	}

	if (cmd) {
		for (opt = 0; cmd_table[opt]; ++opt) {
			if (strcasecmp(cmd_table[opt], cmd) == 0)
				break;
		}
		if (!cmd_table[opt]) {
			printf("Invalid command name: %s\n", cmd);
			printf("Supported commands:\n");
			for (opt = 0; cmd_table[opt]; ++opt) {
				printf("\t%s\n", cmd_table[opt]);
			}
			result = EXIT_FAILURE;
			goto error;
		}
		result = dispatch(&conn, opt, args, args_size, db) ? EXIT_FAILURE : EXIT_SUCCESS;
	}

	if (end) {
		fprintf(stdout, "Tearing down session with id %s\n", conn.session_id);
		if (configd_sess_teardown(&conn, &err) == -1) {
			fprintf(stderr, "Unable to tear down session\n");
			dump_error(&err);
			result = EXIT_FAILURE;
			goto error;
		}
		fprintf(stdout, "Session %s torn down\n", conn.session_id);
	}

error:
	configd_close_connection(&conn);
	free(args);
	fprintf(stdout, "Done\n");
	exit(result);
}
