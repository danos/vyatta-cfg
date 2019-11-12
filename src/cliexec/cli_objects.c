/*
 * Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
 *
 * Copyright (c) 2014, 2016 by Brocade Communications Systems, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>
#include <stdarg.h>
#include "cli_val.h"
#include "cli_parse.h"
#include <regex.h>

#include "cli_objects.h"

/************************ Storage area: *****************/

static char *at_string=NULL;
static boolean in_delete_action=FALSE;
static valstruct cli_value;
static boolean in_commit=FALSE; /* TRUE if in commit program*/
static boolean in_exec=FALSE; /* TRUE if in exec */
static first_seg f_seg_a;
static first_seg f_seg_c;
static first_seg f_seg_m;

/******************** Accessors: ************************/

static char at_buffer[1024]= {0};
static char cfg_path[4096] = {0};

/* the string to use as $VAR(@), must be set
   before call to expand_string */
char* get_at_string(void)
{
	if(at_string) {
		return at_string;
	} else {
		return at_buffer;
	}
}

void set_at_string(char* s)
{
	if(s!=at_buffer) {
		at_string=s;
	} else {
		at_string=NULL;
	}
}

/* the string to use to resolve varrefs, must be set
   before call to expand_string */
const char *get_cfg_path(void)
{
	return cfg_path;
}

void set_cfg_path(const char *s)
{
	if (!s)
		return;
	strncpy(cfg_path, s, sizeof(cfg_path) - 1);
}

boolean is_in_delete_action(void)
{
	return in_delete_action;
}

void set_in_delete_action(boolean b)
{
	in_delete_action=b;
}

boolean is_in_commit(void)
{
	return in_commit;
}

void set_in_commit(boolean b)
{
	in_commit=b;
}

void set_in_exec(boolean b)
{
	in_exec=b;
}

valstruct* get_cli_value_ptr(void)
{
	return &cli_value;
}

first_seg* get_f_seg_a_ptr(void)
{
	return &f_seg_a;
}

first_seg* get_f_seg_c_ptr(void)
{
	return &f_seg_c;
}

first_seg* get_f_seg_m_ptr(void)
{
	return &f_seg_m;
}

#ifdef unused
static char *
get_elevp(void)
{
	static char elevp_buffer[2049];
	static char* elevp=NULL;

	if(elevp==NULL) {

		const char* tmp=getenv(ENV_EDIT_LEVEL);

		if(tmp) {
			strncpy(elevp_buffer,tmp,sizeof(elevp_buffer)-1);
			elevp=elevp_buffer;
		}
	}

	return elevp;
}

static char *
get_tlevp(void)
{
	static char tlevp_buffer[2049];
	static char* tlevp=NULL;

	if(tlevp==NULL) {

		const char* tmp=getenv(ENV_TEMPLATE_LEVEL);

		if(tmp) {
			strncpy(tlevp_buffer,tmp,sizeof(tlevp_buffer)-1);
			tlevp=tlevp_buffer;
		}
	}

	return tlevp;
}
#endif
