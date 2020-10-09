/*
 * Copyright (c) 2019-2020, AT&T Intellectual Property.
 * All rights reserved.
 *
 * Copyright (c) 2014-2016 by Brocade Communications Systems, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 */

#include <argz.h>
#include <errno.h>
#include <getopt.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <unistd.h>

#include <rpc.h>
#include <vyatta-util/map.h>
#include <vyatta-util/vector.h>
#include <vyatta-util/paths.h>

#include "error.h"
#include "connect.h"
#include "node.h"
#include "error.h"
#include "file.h"
#include "session.h"
#include "template.h"
#include "transaction.h"


/* This program provides an API for shell scripts (e.g., snippets in
 * templates, standalone scripts, etc.) to access the CLI cstore library.
 * It is installed in "/opt/vyatta/sbin", but a symlink "/bin/cli-shell-api"
 * is installed by the package so that it can be invoked simply as
 * "cli-shell-api" (as long as "/bin" is in "PATH").
 *
 * The API functions communicate with the caller using a combination of
 * output and exit status. For example, a "boolean" function "returns true"
 * by exiting with status 0, and non-zero exit status means false. The
 * functions are documented below when necessary.
 */


/* options */
/* showCfg options */
int op_show_active_only = 0;
int op_show_show_defaults = 0;
int op_show_hide_secrets = 0;
int op_show_working_only = 0;
int op_show_context_diff = 0;
int op_show_commands = 0;
int op_show_ignore_edit = 0;
int op_show_args_as_path = 0;
char *op_show_cfg1 = NULL;
char *op_show_cfg2 = NULL;

typedef void (*OpFuncT)(struct configd_conn *, const char *);

typedef struct {
	const char *op_name;
	const int op_exact_args;
	const char *op_exact_error;
	const int op_min_args;
	const char *op_min_error;
	bool op_use_edit;
	bool op_insert_edit; // insert after command (e.g., getCompletionEnv show)
	bool op_use_conn; // operation needs configd connection
	OpFuncT op_func;
} OpT;

const char EPATH_ENV[] = "VYATTA_EDIT_LEVEL";
const char SID_ENV[] = "VYATTA_CONFIG_SID";
const char EDIT_FMT[] = "export VYATTA_EDIT_LEVEL='%s'; "
	"export PS1='[edit%s%s]\\n\\u@\\h# ';";


const char *ACTIVE_CFG = "@ACTIVE";
const char *WORKING_CFG = "@WORKING";

/* outputs an environment string to be "eval"ed */
static void
getSessionEnv(struct configd_conn *conn, const char *args)
{
	char *buf;

	configd_set_session_id(conn, args);
	buf = configd_sess_get_env(conn, NULL);
	if (!buf)
		exit(EXIT_FAILURE);
	printf("%s", buf);
	free(buf);
}

/* outputs an environment string to be "eval"ed */
static void
getEditEnv(struct configd_conn *conn, const char *args)
{
	struct configd_error err;
	char *buf = configd_edit_get_env(conn, args, &err);
	if (!buf) {
		fprintf(stderr, "%s\n", err.text);
		exit(EXIT_FAILURE);
	}
	printf("%s", buf);
	free(buf);
}

// read current env, and if value go up 2 levels otherwise go up one
/* outputs an environment string to be "eval"ed */
static void
getEditUpEnv(struct configd_conn *conn, const char *args)
{
	char *parent;
	char *epath_up = NULL;
	size_t epath_len = 0;
	char *epath_env = getenv(EPATH_ENV);

	if (!epath_env)
		exit(EXIT_FAILURE);

	if (strcmp(epath_env, "/") == 0) {
		fprintf(stderr, "Already at the top level\n");
		exit(EXIT_FAILURE);
	}

	parent = dirname(epath_env);

	if (argz_create_sep(parent, '/', &epath_up, &epath_len))
		exit(EXIT_FAILURE);
	argz_stringify(epath_up, epath_len, ' ');
	if (configd_node_get_type(conn, epath_up, NULL) == NODE_TYPE_TAG) {
		free(epath_up);
		epath_env = parent;
		parent = dirname(epath_env);
		epath_up = NULL;
		epath_len = 0;
		if (argz_create_sep(parent, '/', &epath_up, &epath_len))
			exit(EXIT_FAILURE);
		argz_stringify(epath_up, epath_len, ' ');
	}

	if (strlen(epath_up))
		printf(EDIT_FMT, parent, " ", epath_up);
	else
		printf(EDIT_FMT, parent, "", "");
	free(epath_up);
}

/* outputs an environment string to be "eval"ed */
static void
getEditResetEnv(struct configd_conn *conn, const char *args)
{
	printf(EDIT_FMT, "/", "", "");
}

static void
editLevelAtRoot(struct configd_conn *conn, const char *args)
{

	char *epath_env = getenv(EPATH_ENV);

	if (!epath_env || (strcmp(epath_env, "/") != 0))
		exit(EXIT_FAILURE);
}

/* outputs an environment string to be "eval"ed */
static void
getCompletionEnv(struct configd_conn *conn, const char *args)
{
	char *buf = configd_node_get_complete_env(conn, args, NULL);
	if (!buf)
		exit(EXIT_FAILURE);
	printf("%s", buf);
	free(buf);
}

/* outputs a string */
static void
getEditLevelStr(struct configd_conn *conn, const char *args)
{
	char *epath = NULL;
	size_t epath_len = 0;
	char *epath_env = getenv(EPATH_ENV);

	if (!epath_env)
		exit(EXIT_FAILURE);
	if (argz_create_sep(epath_env+1, '/', &epath, &epath_len))
		exit(EXIT_FAILURE);
	argz_stringify(epath, epath_len, ' ');
	printf("%s", epath);
	free(epath);
}

static void
markSessionUnsaved(struct configd_conn *conn, const char *args)
{
	if (configd_sess_mark_unsaved(conn, NULL) != 0)
		exit(EXIT_FAILURE);
}

static void
unmarkSessionUnsaved(struct configd_conn *conn, const char *args)
{
	if (configd_sess_mark_saved(conn, NULL) != 0)
		exit(EXIT_FAILURE);
}

static void
sessionUnsaved(struct configd_conn *conn, const char *args)
{
	if (configd_sess_saved(conn, NULL) != 0)
		exit(EXIT_FAILURE);
}

static void
sessionChanged(struct configd_conn *conn, const char *args)
{
	if (configd_sess_changed(conn, NULL) != 1)
		exit(EXIT_FAILURE);
}

static void
teardownSession(struct configd_conn *conn, const char *args)
{
	if (configd_sess_teardown(conn, NULL) == -1)
		exit(EXIT_FAILURE);
}

static void
setupSession(struct configd_conn *conn, const char *args)
{
	if (configd_sess_setup(conn, NULL) == -1)
		exit(EXIT_FAILURE);
}

static void
setupSharedSession(struct configd_conn *conn, const char *args)
{
	if (configd_sess_setup_shared(conn, NULL) == -1)
		exit(EXIT_FAILURE);
}

static void
inSession(struct configd_conn *conn, const char *args)
{
	if (configd_sess_exists(conn, NULL) != 1)
		exit(EXIT_FAILURE);
}

/* same as exists() in Perl API */
static void
exists(struct configd_conn *conn, const char *args)
{
	if (configd_node_exists(conn, CANDIDATE, args, NULL) != 1)
		exit(EXIT_FAILURE);
}

/* same as existsOrig() in Perl API */
static void
existsActive(struct configd_conn *conn, const char *args)
{
	if (configd_node_exists(conn, RUNNING, args, NULL) != 1)
		exit(EXIT_FAILURE);
}

/* same as isEffective() in Perl API */
static void
existsEffective(struct configd_conn *conn, const char *args)
{
	if (configd_node_exists(conn, EFFECTIVE, args, NULL) != 1)
		exit(EXIT_FAILURE);
}

/* isMulti */
static void
isMulti(struct configd_conn *conn, const char *args)
{
	if (configd_node_get_type(conn, args, NULL) != NODE_TYPE_MULTI)
		exit(EXIT_FAILURE);
}

/* isTag */
static void
isTag(struct configd_conn *conn, const char *args)
{
	if (configd_node_get_type(conn, args, NULL) != NODE_TYPE_TAG)
		exit(EXIT_FAILURE);
}

/* isValue */
static void
isValue(struct configd_conn *conn, const char *args)
{
	int is_value;
	const char *type;
	struct map *m = configd_tmpl_get(conn, args, NULL);
	if (!m)
		exit(EXIT_FAILURE);
	type = map_get(m, "is_value");
	is_value = type && (strcmp(type, "1") == 0);
	free(m);
	if (!is_value)
		exit(EXIT_FAILURE);
}

/* isSecret */
static void
isSecret(struct configd_conn *conn, const char *args)
{
	map *tmpl = configd_tmpl_get(conn, args, NULL);
	if (tmpl == NULL) {
		exit(EXIT_FAILURE);
	}
	const char *val = map_get(tmpl, "secret");
	if (val != NULL && strcmp(val, "1") == 0) {
		exit(EXIT_SUCCESS);
	}
	free(tmpl);
	exit(EXIT_FAILURE);
}

/* isLeaf */
static void
isLeaf(struct configd_conn *conn, const char *args)
{
	if (configd_node_get_type(conn, args, NULL) != NODE_TYPE_LEAF)
		exit(EXIT_FAILURE);
}

static void
getNodeType(struct configd_conn *conn, const char *args)
{
	const char *typestr[] = {
		"leaf", "multi", "non-leaf", "tag"
	};
	int type = configd_node_get_type(conn, args, NULL);
	if ((type < NODE_TYPE_LEAF) || (type > NODE_TYPE_TAG))
		exit(EXIT_FAILURE);
	printf("%s", typestr[type]);
}

static void list_nodes(struct configd_conn *conn, int db, const char *args)
{
	const char *str = NULL;
	struct vector *v = configd_node_get(conn, db, args, NULL);
	if (!v)
		exit(EXIT_FAILURE);

	
	if ((str = vector_next(v, str)) != NULL) {
		printf("'%s'", str);
		while ((str = vector_next(v, str))) {
			printf(" '%s'", str);
		}
	}
	vector_free(v);
}

/* same as listNodes() in Perl API.
 *
 * outputs a string representing multiple nodes. this string MUST be
 * "eval"ed into an array of nodes. e.g.,
 *
 *   values=$(cli-shell-api listNodes interfaces)
 *   eval "nodes=($values)"
 *
 * or a single step:
 *
 *   eval "nodes=($(cli-shell-api listNodes interfaces))"
 */
static void
listNodes(struct configd_conn *conn, const char *args)
{
	list_nodes(conn, CANDIDATE, args);
}

/* same as listOrigNodes() in Perl API.
 *
 * outputs a string representing multiple nodes. this string MUST be
 * "eval"ed into an array of nodes. see listNodes above.
 */
static void
listActiveNodes(struct configd_conn *conn, const char *args)
{
	list_nodes(conn, RUNNING, args);
}

/* same as listEffectiveNodes() in Perl API.
 *
 * outputs a string representing multiple nodes. this string MUST be
 * "eval"ed into an array of nodes. see listNodes above.
 */
static void
listEffectiveNodes(struct configd_conn *conn, const char *args)
{
	list_nodes(conn, EFFECTIVE, args);
}

/* same as returnValue() in Perl API. outputs a single unquoted string. */
static void
returnValue(struct configd_conn *conn, const char *args)
{
	struct vector *v = configd_node_get(conn, CANDIDATE, args, NULL);
	if (!v || (vector_count(v) == 0))
		exit(EXIT_FAILURE);

	printf("%s", vector_next(v, NULL));
	vector_free(v);
}

/* same as returnOrigValue() in Perl API. outputs a string. */
static void
returnActiveValue(struct configd_conn *conn, const char *args)
{
	list_nodes(conn, RUNNING, args);
}

/* same as returnEffectiveValue() in Perl API. outputs a string. */
static void
returnEffectiveValue(struct configd_conn *conn, const char *args)
{
	list_nodes(conn, EFFECTIVE, args);
}

/* same as returnValues() in Perl API.
 *
 * outputs a string representing multiple values. this string MUST be
 * "eval"ed into an array of values. see listNodes above.
 *
 * note that success/failure can be checked using the two-step invocation
 * above. e.g.,
 *
 *   if valstr=$(cli-shell-api returnValues system ntp-server); then
 *     # got the values
 *     eval "values=($valstr)"
 *     ...
 *   else
 *     # failed
 *     ...
 *   fi
 *
 * in most cases, the one-step invocation should be sufficient since a
 * failure would result in an empty array after the eval.
 */
static void
returnValues(struct configd_conn *conn, const char *args)
{
	list_nodes(conn, CANDIDATE, args);
}

/* same as returnOrigValues() in Perl API.
 *
 * outputs a string representing multiple values. this string MUST be
 * "eval"ed into an array of values. see returnValues above.
 */
static void
returnActiveValues(struct configd_conn *conn, const char *args)
{
	list_nodes(conn, RUNNING, args);
}

/* same as returnEffectiveValues() in Perl API.
 *
 * outputs a string representing multiple values. this string MUST be
 * "eval"ed into an array of values. see returnValues above.
 */
static void
returnEffectiveValues(struct configd_conn *conn, const char *args)
{
	list_nodes(conn, EFFECTIVE, args);
}

/* checks if specified path is a valid "template path" *without* checking
 * the validity of any "tag values" along the path.
 */
static void
validateTmplPath(struct configd_conn *conn, const char *args)
{
	if (configd_tmpl_validate_path(conn, args, NULL) != 1)
		exit(EXIT_FAILURE);
}

/* checks if specified path is a valid "template path", *including* the
 * validity of any "tag values" along the path.
 */
static void
validateTmplValPath(struct configd_conn *conn, const char *args)
{
	if (configd_tmpl_validate_values(conn, args, NULL) != 1)
		exit(EXIT_FAILURE);
}

static void getTmplAllowed(struct configd_conn *conn, const char *args)
{
	const char *str = NULL;
	struct configd_error err;
	struct vector *v = configd_tmpl_get_allowed(conn, args, &err);
	if (!v) {
		if (err.text != NULL)
			printf("%s\n", err.text);
		exit(EXIT_FAILURE);
	}

	if ((str = vector_next(v, str)) != NULL) {
		printf("'%s'", str);
		while ((str = vector_next(v, str))) {
			printf(" '%s'", str);
		}
	}
	vector_free(v);
}

static void
getSchema(struct configd_conn *conn, const char *args)
{
	char *argz = NULL;
	size_t argz_len = 0;
	char **argv = (char**)calloc(1, (argz_count(argz, argz_len) + 1) * sizeof(char *));

	if (!argv)
		exit(EXIT_FAILURE);

	argz_create_sep(args, ' ', &argz, &argz_len);
	argz_extract(argz, argz_len, argv);

	if (argz_len < 2) {
		free(argv);
		exit(EXIT_FAILURE);
	}
	
	char *result = configd_schema_get(conn, argv[0], argv[1], NULL);
	free(argv);
	if (!result) {
		exit(EXIT_FAILURE);
	}
	printf("%s", result);
	free(result);
}

static int get_show_flags(void)
{
	int flags = 0;

	if (op_show_show_defaults)
		flags |= SHOWF_DEFAULTS;
	if (op_show_hide_secrets)
		flags |= SHOWF_HIDE_SECRETS;
	if (op_show_context_diff)
		flags |= SHOWF_CONTEXT_DIFF;
	if (op_show_commands)
		flags |= SHOWF_COMMANDS;
	if (op_show_ignore_edit)
		flags |= SHOWF_IGNORE_EDIT;

	return flags;
}


static void
showCfg(struct configd_conn *conn, const char *args)
{
	char *result;
	bool inSession = configd_sess_exists(conn, NULL);
	bool active_only = (!inSession || op_show_active_only);
	bool working_only = (inSession && op_show_working_only);
	int flags = get_show_flags();

	if (active_only)
		/* just show the active config (no diff) */
		result = configd_show(conn, RUNNING, args, ACTIVE_CFG, ACTIVE_CFG, flags, NULL);
	else if (working_only)
		result = configd_show(conn, CANDIDATE, args, WORKING_CFG, WORKING_CFG, flags, NULL);
	else
		result = configd_show(conn, CANDIDATE, args, ACTIVE_CFG, WORKING_CFG, flags, NULL);

	if (!result)
		exit(EXIT_FAILURE);
	printf("%s", result);
	free(result);
}

/* new "show" API providing superset of functionality of showCfg above.
 * available command-line options (all are optional):
 *   --show-cfg1 <cfg1> --show-cfg2 <cfg2>
 *       specify the two configs to be diffed (must specify both)
 *       <cfg1>: "@ACTIVE", "@WORKING", or config file name
 *       <cfg2>: "@ACTIVE", "@WORKING", or config file name
 *
 *       if not specified, default is cfg1="@ACTIVE" and cfg2="@WORKING",
 *       i.e., same as "traditional show"
 *   --show-active-only
 *       only show active config (i.e., cfg1="@ACTIVE" and cfg2="@ACTIVE")
 *   --show-working-only
 *       only show working config (i.e., cfg1="@WORKING" and cfg2="@WORKING")
 *   --show-show-defaults
 *       display "default" values
 *   --show-hide-secrets
 *       hide "secret" values when displaying
 *   --show-context-diff
 *       show "context diff" between two configs
 *   --show-commands
 *       show output in "commands"
 *   --show-ignore-edit
 *       don't use the edit level in environment
 *
 * note that when neither cfg1 nor cfg2 specifies a config file, the "args"
 * argument specifies the root path for the show output, and the "edit level"
 * in the environment is used.
 *
 * on the other hand, if either cfg1 or cfg2 specifies a config file, then
 * both "args" and "edit level" are ignored.
 */
static void
showConfig(struct configd_conn *conn, const char *args)
{
	const char *cfg1 = ACTIVE_CFG;
	const char *cfg2 = WORKING_CFG;
	char *result;
	int flags = get_show_flags();

	if (op_show_active_only) {
		cfg2 = cfg1;
	} else if (op_show_working_only) {
		cfg1 = cfg2;
	} else if (op_show_cfg1 && op_show_cfg2) {
		cfg1 = op_show_cfg1;
		cfg2 = op_show_cfg2;
	} else {
		/* default */
	}

	int db = RUNNING;
	if (configd_sess_exists(conn, NULL)) {
		db = CANDIDATE;
	}

	result = configd_show(conn, db, args, cfg1, cfg2, flags, NULL);
	if (!result)
		exit(EXIT_FAILURE);
	printf("%s", result);
	free(result);
}

static void
loadFile(struct configd_conn *conn, const char *args)
{
	struct configd_error err;
	if (configd_load(conn, args, &err) != 1) {
		if (err.text != NULL) {
			printf("%s\n", err.text);
		}
		exit(EXIT_FAILURE);
	}
}

static void
loadFileReportWarnings(struct configd_conn *conn, const char *args)
{
	struct configd_error err;
	if (configd_load_report_warnings(conn, args, &err) != 1) {
		if (err.text != NULL) {
			printf("%s\n", err.text);
		}
		exit(EXIT_FAILURE);
	}
	if (err.text != NULL) {
		printf("%s\n", err.text);
	}
}

static void
mergeFile(struct configd_conn *conn, const char *args)
{
	struct configd_error err;
	if (configd_merge(conn, args, &err) != 1) {
		if (err.text != NULL) {
			printf("%s\n", err.text);
		}
		exit(EXIT_FAILURE);
	}
}

static void
saveConfig(struct configd_conn *conn, const char * args)
{
	struct configd_error err;
	char *result;
	result = configd_save(conn, args, &err);
	if (!result) {
		if (err.text != NULL)
			printf("%s\n", err.text);
		exit(EXIT_FAILURE);
	}
	printf("%s", result);
	free(result);
}


// Deprecated API
/* output the "pre-commit hook dir" */
static void
getPreCommitHookDir(struct configd_conn *conn, const char *args)
{
	//syslog(LOG_WARNING, "%s: Deprecated API", __func__);
	printf("/etc/commit/pre-hooks.d");
}


// Deprecated API
/* output the "post-commit hook dir" */
static void
getPostCommitHookDir(struct configd_conn *conn, const char *args)
{
	//syslog(LOG_WARNING, "%s: Deprecated API", __func__);
	printf("/etc/commit/post-hooks.d");
}

static void
getTree(struct configd_conn *conn, const char *args)
{
	char *str = configd_tree_get(conn, CANDIDATE, args, NULL);
	if (!str)
		exit(EXIT_FAILURE);
	
	printf("%s\n", str);
	free(str);
}

static void
getActiveTree(struct configd_conn *conn, const char *args)
{
	char *str = configd_tree_get(conn, RUNNING, args, NULL);
	if (!str)
		exit(EXIT_FAILURE);
	
	printf("%s\n", str);
	free(str);
}


static void
migrateFile(struct configd_conn *conn, const char *filename)
{
	struct configd_error err = {0};
	char *str = configd_file_migrate(conn, filename, &err);
	if (!str) {
		if (err.text != NULL) {
			fprintf(stderr, "%s", err.text);
		} else {
			fprintf(stderr, "Migration failed\n");
		}
		configd_error_free(&err);
		exit(EXIT_FAILURE);
	}
	if (strcmp(str, "") != 0) {
		printf("%s", str);
	}
	configd_error_free(&err);
	exit(EXIT_SUCCESS);
}


#define OP(name, exact, exact_err, min, min_err, use_edit, insert_edit, use_conn)	\
	{ #name, exact, exact_err, min, min_err, use_edit, insert_edit, use_conn, &name }

#define USE_CONN true
#define USE_EDIT true
#define INS_EDIT true

static int op_idx = -1;
static OpT ops[] = {
	OP(getSessionEnv, 1, "Must specify session ID", -1, NULL, true, false, USE_CONN),
	OP(getEditEnv, -1, NULL, 1, "Must specify config path", true, false, USE_CONN),
	OP(getEditUpEnv, 0, "No argument expected", -1, NULL, true, false, USE_CONN),
	OP(getEditResetEnv, 0, "No argument expected", -1, NULL, true, false, !USE_CONN),
	OP(editLevelAtRoot, 0, "No argument expected", -1, NULL, true, false, !USE_CONN),
	OP(getCompletionEnv, -1, NULL,
	2, "Must specify command and at least one component", true, true, USE_CONN),
	OP(getEditLevelStr, 0, "No argument expected", -1, NULL, true, true, !USE_CONN),

	OP(markSessionUnsaved, 0, "No argument expected", -1, NULL, false, false, USE_CONN),
	OP(unmarkSessionUnsaved, 0, "No argument expected", -1, NULL, false, false, USE_CONN),
	OP(sessionUnsaved, 0, "No argument expected", -1, NULL, false, false, USE_CONN),
	OP(sessionChanged, 0, "No argument expected", -1, NULL, false, false, USE_CONN),

	OP(teardownSession, 0, "No argument expected", -1, NULL, false, false, USE_CONN),
	OP(setupSession, 0, "No argument expected", -1, NULL, false, false, USE_CONN),
	OP(setupSharedSession, 0, "No argument expected", -1, NULL, false, false, USE_CONN),
	OP(inSession, 0, "No argument expected", -1, NULL, false, false, USE_CONN),

	OP(exists, -1, NULL, 1, "Must specify config path", false, false, USE_CONN),
	OP(existsActive, -1, NULL, 1, "Must specify config path", false, false, USE_CONN),
	OP(existsEffective, -1, NULL, 1, "Must specify config path", false, false, USE_CONN),

	OP(listNodes, -1, NULL, -1, NULL, false, false, USE_CONN),
	OP(listActiveNodes, -1, NULL, -1, NULL, false, false, USE_CONN),
	OP(listEffectiveNodes, -1, NULL, 1, "Must specify config path", false, false, USE_CONN),

	OP(isMulti, -1, NULL, 1, "Must specify config path", false, false, USE_CONN),
	OP(isTag,  -1, NULL, 1, "Must specify config path", false, false, USE_CONN),
	OP(isLeaf, -1, NULL, 1, "Must specify config path", false, false, USE_CONN),
	OP(isValue, -1, NULL, 1, "Must specify config path", false, false, USE_CONN),
	OP(isSecret, -1, NULL, 1, "Must specify config path", false, false, USE_CONN),
	OP(getNodeType, -1, NULL, 1, "Must specify config path", true, false, USE_CONN),
	OP(getSchema, -1, NULL, -1, NULL, false, false, USE_CONN),

	OP(returnValue, -1, NULL, 1, "Must specify config path", false, false, USE_CONN),
	OP(returnActiveValue, -1, NULL, 1, "Must specify config path", false, false, USE_CONN),
	OP(returnEffectiveValue, -1, NULL, 1, "Must specify config path", false, false, USE_CONN),

	OP(returnValues, -1, NULL, 1, "Must specify config path", false, false, USE_CONN),
	OP(returnActiveValues, -1, NULL, 1, "Must specify config path", false, false, USE_CONN),
	OP(returnEffectiveValues, -1, NULL, 1, "Must specify config path", false, false, USE_CONN),

	OP(validateTmplPath, -1, NULL, 1, "Must specify config path", true, false, USE_CONN),
	OP(validateTmplValPath, -1, NULL, 1, "Must specify config path", false, false, USE_CONN),
	OP(getTmplAllowed, -1, NULL, 1, "Must specify config path", false, false, USE_CONN),

	OP(showCfg, -1, NULL, -1, NULL, !USE_EDIT, !INS_EDIT, USE_CONN),
	OP(showConfig, -1, NULL, -1, NULL, !USE_EDIT, !INS_EDIT, USE_CONN),
	OP(loadFile, 1, "Must specify config file", -1, NULL, false, false, USE_CONN),
	OP(loadFileReportWarnings, 1, "Must specify config file", -1, NULL, false, false, USE_CONN),
	OP(mergeFile, 1, "Must specify config file", -1, NULL, false, false, USE_CONN),
	OP(saveConfig, -1, NULL, -1, NULL, false, false, USE_CONN),

	OP(getPreCommitHookDir, 0, "No argument expected", -1, NULL, false, false, !USE_CONN),
	OP(getPostCommitHookDir, 0, "No argument expected", -1, NULL, false, false, !USE_CONN),

	OP(getTree, -1, NULL, -1, NULL, true, false, USE_CONN),
	OP(getActiveTree, -1, NULL, -1, NULL, true, false, USE_CONN),

	OP(migrateFile, -1, NULL, 1, "Must specify filename", !USE_EDIT, !INS_EDIT, USE_CONN),

	{NULL, -1, NULL, -1, NULL, false, false, false}
};
#define OP_exact_args  ops[op_idx].op_exact_args
#define OP_min_args    ops[op_idx].op_min_args
#define OP_exact_error ops[op_idx].op_exact_error
#define OP_min_error   ops[op_idx].op_min_error
#define OP_use_edit    ops[op_idx].op_use_edit
#define OP_insert_edit ops[op_idx].op_insert_edit
#define OP_use_conn    ops[op_idx].op_use_conn
#define OP_func        ops[op_idx].op_func
#define OP_name        ops[op_idx].op_name

enum {
	SHOW_CFG1 = 1,
	SHOW_CFG2
};

struct option options[] = {
	{"path", no_argument, &op_show_args_as_path, 1},
	{"show-active-only", no_argument, &op_show_active_only, 1},
	{"show-show-defaults", no_argument, &op_show_show_defaults, 1},
	{"show-hide-secrets", no_argument, &op_show_hide_secrets, 1},
	{"show-working-only", no_argument, &op_show_working_only, 1},
	{"show-context-diff", no_argument, &op_show_context_diff, 1},
	{"show-commands", no_argument, &op_show_commands, 1},
	{"show-ignore-edit", no_argument, &op_show_ignore_edit, 1},
	{"show-cfg1", required_argument, NULL, SHOW_CFG1},
	{"show-cfg2", required_argument, NULL, SHOW_CFG2},
	{NULL, 0, NULL, 0}
};

int
main(int argc, char **argv)
{
	/* handle options first */
	int c = 0;
	struct configd_conn conn;
	char *epath = NULL;
	size_t epath_len = 0;
	char *args = NULL;
	size_t args_len = 0;
	char *buf;
	char *path = NULL;
	int result = EXIT_SUCCESS;

	while ((c = getopt_long(argc, argv, "", options, NULL)) != -1) {
		switch (c) {
		case SHOW_CFG1:
			op_show_cfg1 = strdup(optarg);
			break;
		case SHOW_CFG2:
			op_show_cfg2 = strdup(optarg);
			break;
		default:
			break;
		}
	}
	int nargs = argc - optind - 1;
	char *oname = argv[optind];
	char **nargv = &(argv[optind + 1]);

	int i = 0;
	if (nargs < 0) {
		fprintf(stderr, "Must specify operation\n");
		exit(EXIT_FAILURE);
	}
	while (ops[i].op_name) {
		if (strcmp(oname, ops[i].op_name) == 0) {
			op_idx = i;
			break;
		}
		++i;
	}
	if (op_idx == -1) {
		fprintf(stderr, "Invalid operation\n");
		exit(EXIT_FAILURE);
	}
	if (OP_exact_args >= 0 && nargs != OP_exact_args) {
		fprintf(stderr, "%s\n", OP_exact_error);
		exit(EXIT_FAILURE);
	}
	if (OP_min_args >= 0 && nargs < OP_min_args) {
		fprintf(stderr, "%s\n", OP_min_error);
		exit(EXIT_FAILURE);
	}

	/*Commands that do not requre edit level or connection*/
	if (!OP_use_conn && !OP_use_edit) {
		OP_func(NULL, NULL);
		exit(EXIT_SUCCESS);
	}

	buf = getenv(EPATH_ENV);
	if (OP_use_edit && buf) {
		if (argz_create_sep(buf, '/', &epath, &epath_len))
			exit(EXIT_FAILURE);
	}

	if (argz_create(nargv, &args, &args_len))
		exit(EXIT_FAILURE);

	if (configd_open_connection(&conn) == -1) {
		fprintf(stderr, "Unable to open connection: %s\n", strerror(errno));
		result = EXIT_FAILURE;
		goto done;
	}

	buf = getenv(SID_ENV);
	if (buf)
		configd_set_session_id(&conn, buf);

	/*Commands that do not require edit level but require connection*/
	if (!OP_use_edit) {
		if (OP_func != saveConfig && OP_func != loadFile
			&& OP_func != loadFileReportWarnings
		    && OP_func != showCfg && OP_func != showConfig
		    && OP_func != getSchema && OP_func != migrateFile) {
			path = args_to_path(args, args_len);
			OP_func(&conn, path);
			goto done_conn;
		}

		if (OP_func == showCfg && op_show_args_as_path) {
			path = args_to_path(args, args_len);
			OP_func(&conn, path);
			goto done_conn;
		}
		argz_stringify(args, args_len, ' ');
		OP_func(&conn, args);
		goto done_conn;
	}

	if (epath && strlen(epath)) {
		if (OP_insert_edit) {
			if (argz_insert(&epath, &epath_len, epath, args)) {
				result = EXIT_FAILURE;
				goto done_conn;
			}
			argz_delete(&args, &args_len, args);
		}
		if (argz_append(&epath, &epath_len, args, args_len)) {
			result = EXIT_FAILURE;
			goto done_conn;
		}
		free(args);
		args = epath;
		args_len = epath_len;
	}

	path = args_to_path(args, args_len);
	argz_stringify(args, args_len, ' ');

	/*Commands requiring edit level*/
	if (OP_func == getSessionEnv) {
		OP_func(&conn, args);
		goto done_conn;
	}
	/* call the op function */
	OP_func(&conn, path);

done_conn:
	configd_close_connection(&conn);
done:
	free(args);
	free(path);
	exit(result);
}

