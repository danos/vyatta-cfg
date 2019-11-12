/* Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
 *
 * Copyright (c) 2014-2016 by Brocade Communications Systems, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <iostream>
#include <fstream>
#include <sstream>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <connect.h>
#include <cpath.hpp>
#include <ctemplate.hpp>
#include <log.h>
#include "../cli_cstore.h"
#include "cli_objects.h"
#include "cpath.hpp"

#ifdef __cplusplus
extern "C" {
#endif

extern char *program_invocation_short_name;

int expand_string(const char *);
const char *get_exe_string(void);

#ifdef __cplusplus
}
#endif

enum {
	SCRIPT_UNKNOWN,
	SCRIPT_OTHER,
	SCRIPT_EXPR
};

/* Where all the node scripts live */
static const char tmplscripts[] = "/opt/vyatta/share/tmplscripts";

static const char SID_ENV[] = "VYATTA_CONFIG_SID";
static const char ACTION_ENV[] = "CONFIGD_EXT";
static const char NODE_PATH[] = "CONFIGD_PATH";
static const char COMMIT_ACTION_ENV[] = "COMMIT_ACTION";

static struct configd_conn conn;
static int no_shell;

static void connect(void)
{
	char *sid;
	if (configd_open_connection(&conn)) {
		std::cerr << "Unable to connect to configd\n";
		exit(EXIT_FAILURE);
	}

	sid = getenv(SID_ENV);
	if (sid)
		configd_set_session_id(&conn, sid);
}

static int has_she_bang(const char *name)
{
	FILE *script;
	char line[3];

	script = fopen(name, "r");
	if (!script)
		return -1;
	if (fgets(line, sizeof(line), script) == NULL) {
		fclose(script);
		return -1;
	}
	fclose(script);
	return (::strncmp(line, "#!", 2) == 0);
}

static int get_script_type(const char *progname, const char *name)
{
	if (!progname || !name)
		return SCRIPT_UNKNOWN;

	if (::strcmp(progname, "cliexpr") == 0)
		return SCRIPT_EXPR;
	if (has_she_bang(name) == 1)
		return SCRIPT_OTHER;
	return SCRIPT_UNKNOWN;
}

static void read_script(const std::string &fname, std::string &script)
{
	std::ifstream scriptfile(fname);
	std::stringstream buffer;
	buffer << scriptfile.rdbuf();
	script = buffer.str();
}

static char *extract_at_string(const std::string &cpath)
{
	Cpath path(cpath);
	std::string at_string = path.back();
	return strdup(at_string.c_str());
}

static vtw_act_type get_action_type(void)
{
	const char *action_env = getenv(ACTION_ENV);
	if (!action_env)
		return top_act;

	if (strcasecmp(action_env, "allowed") == 0)
		return allowed_act;
	if (strcasecmp(action_env, "delete") == 0)
		return delete_act;
	if (strcasecmp(action_env, "create") == 0)
		return create_act;
	if (strcasecmp(action_env, "activate") == 0)
		return activate_act;
	if (strcasecmp(action_env, "update") == 0)
		return update_act;
	if (strcasecmp(action_env, "syntax") == 0)
		return syntax_act;
	if (strcasecmp(action_env, "commit") == 0)
		return commit_act;
	if (strcasecmp(action_env, "begin") == 0)
		return begin_act;
	if (strcasecmp(action_env, "end") == 0)
		return end_act;
	if (strcasecmp(action_env, "sub") == 0)
		return sub_act;
	return top_act;
}

static int is_delete(vtw_act_type action)
{
	char *commit_action = getenv(COMMIT_ACTION_ENV);
	if (!commit_action)
		return action == delete_act;
	return (strcmp(commit_action, "DELETE") == 0
		&& (action == begin_act || action == end_act))
		|| action == delete_act;
}

static int process_cli_script(const std::string &fname, const std::string &cpath)
{
	vtw_def def;

	connect();
	if (parse_def(&def, fname.c_str(), 0)) {
		std::cerr << "Unable to parse node: " << fname << std::endl;
		return EXIT_FAILURE;
	}

	Cpath pcomps(cpath);
	Ctemplate configd_def((struct configd_conn*)var_ref_handle, pcomps.to_path_string().c_str());
	if (!configd_def.get()) {
		std::cerr << "could not get template from configd";
		return EXIT_FAILURE;
	}

	def.def_type = configd_def.getType(1);
	def.def_type2 = configd_def.getType(2);
	def.def_tag = configd_def.isTag() ? 1 : 0;
	def.def_multi = configd_def.isMulti() ? 1 : 0;
	def.tag = configd_def.isTag();
	def.multi = configd_def.isMulti();

	vtw_act_type act_type = get_action_type();
	if (act_type == top_act) {
		std::cerr << "Unable to determine action.\n";
		return EXIT_FAILURE;
	}
	set_in_delete_action(is_delete(act_type));

	vtw_node *actions = def.actions[act_type].vtw_list_head;
	Cpath path(cpath);
	std::string disp_path(" ");
	disp_path += path.to_string();
	disp_path += " ";

	char *at_string = extract_at_string(cpath);
	set_at_string(at_string);
	set_cfg_path(cpath.c_str());
	bool ret = execute_list(actions, &def, NULL);
	return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}

static void process_string(const std::string &str, const std::string &cpath)
{
	static const char *varref_prefix = "$VAR(";
	char *at_string;
	const char *estr;

	if (str.find(varref_prefix) != std::string::npos) {
		connect();
	}
	at_string = extract_at_string(cpath);
	set_at_string(at_string);
	set_cfg_path(cpath.c_str());
	if (expand_string(str.c_str()) != VTWERR_OK) {
		perror("expand_string");
		exit(EXIT_FAILURE);
	}
	estr = get_exe_string();
	free(at_string);
	if (no_shell) {
		std::string cmd("exec ");
		cmd += estr;
		execl("/bin/sh", "sh", "-c", cmd.c_str(), NULL);
	} else {
		execl("/bin/sh", "sh", "-c", estr, NULL);
	}
	perror("execl");
	exit(EXIT_FAILURE);
}

static void process_sh_script(const std::string &fname, const std::string &cpath)
{
	vtw_act_type act_type = get_action_type();
	if (act_type == top_act) {
		std::cerr << "Unable to determine action.\n";
		exit(EXIT_FAILURE);
	}
	set_in_delete_action(is_delete(act_type));

	std::string script;
	read_script(fname, script);
	process_string(script, cpath);
}

static void process_cmd(const std::string &cmd, const std::string &cpath)
{
	vtw_act_type act_type = get_action_type();
	if (act_type == top_act) {
		std::cerr << "Unable to determine action.\n";
		exit(EXIT_FAILURE);
	}
	set_in_delete_action(is_delete(act_type));
	process_string(cmd, cpath);
}

static void usage(int result)
{
	std::cout << "\nUsage: " << program_invocation_short_name
		  << " [-h] [-d] [-c cmd | -s cmd | script]\n\n";
	exit(result);
}

int main(int argc, char *argv[])
{
	int ret = EXIT_SUCCESS;
	int opt;
	const char *env;
	std::string fname;
	std::string cmd;
	std::string cpath;

	set_in_commit(true);

	while ((opt = ::getopt(argc, argv, ":c:ds:h")) != -1)
	{
		switch (opt) {
		case 'h':	      /* Help please */
			usage(EXIT_SUCCESS);
			break;

		case 'c':	      /* Command */
			cmd = optarg;
			break;

		case 'd':	      /* Debug mode */
			msg_use_console(1);
			break;

		case 's':             /* direct command */
			no_shell = 1;
			cmd = optarg;
			break;

		case ':':
			std::cerr << "Option " << optopt << "is missing a parameter; ignoring\n";
			break;

		case '?':
			std::cerr << "Unknown option " << optopt << "; ignoring\n";
			break;
		}
	}

	// If we are not processing a command, make sure we have a
	// script name to process.
	if (!cmd.length()) {
		if (optind >= argc) {
			std::cerr << "Missing script to execute\n";
			usage(EXIT_FAILURE);
		}
		fname = argv[optind++];
	}

	env = getenv(NODE_PATH);
	if (!env) {
		std::cerr << "Missing configuration node path\n";
		usage(EXIT_FAILURE);
	}
	cpath = env;
	var_ref_handle = &conn;
	out_stream = stdout;
	err_stream = stderr;

	if (cmd.length())
		process_cmd(cmd, cpath); /* does not return */

	switch (get_script_type(program_invocation_short_name, fname.c_str())) {
	case SCRIPT_EXPR:
		ret = process_cli_script(fname, cpath);
		break;
	case SCRIPT_OTHER:
		process_sh_script(fname, cpath);
		break;
	default:
		std::cerr << "Unrecognized script " << fname << std::endl;
		ret = EXIT_FAILURE;
		break;
	}
	exit(ret);
}
