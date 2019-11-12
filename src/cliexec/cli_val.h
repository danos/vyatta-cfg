/*
 * Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
 *
 * Copyright (c) 2014, 2016 by Brocade Communications Systems, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 */
#ifndef CLI_DEF_H
#define CLI_DEF_H
#include <stdio.h>

#include "../cli_cstore.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE (!FALSE)
#endif

	/* allocation unit for vals in valstruct */
#define MULTI_ALLOC 5 /* we have room if cnt%MULTI_ALLOC != 0 */

	typedef enum {
		do_del_mode,
		del_mode,
		create_mode,
		update_mode
	}
	vtw_cmode;

	typedef enum {
		EQ_COND = 1,
		NE_COND,
		LT_COND,
		LE_COND,
		GT_COND,
		GE_COND,
		IN_COND,
		TOP_COND
	} vtw_cond_e;
	/* IN_COND is like EQ for singular compare, but OR for multivalue right operand */

	typedef struct {
		const char *f_segp;
		int   f_seglen;
		int   f_segoff;
	} first_seg;
	/* the first segment might be ADIR, or CDIR, or MDIR
	   we reserve space large enough for any one.
	   If the shorter one is used, it right aligned.
	   path points to the start of the current first
	   segment
	*/
	typedef struct {
		char *path_buf;       /* path buffer */
		char *path;           /* path */
		int   path_len;       /* path length used */
		int   path_alloc;     /* allocated - 1*/
		int  *path_ends;       /* path ends for dif levels*/
		int   path_lev;       /* how many used */
		int   path_ends_alloc; /* how many allocated */
		int   print_offset;  /* for additional optional output information */
	} vtw_path;  /* vyatta tree walk */

	extern int char2val(const vtw_def *def, char *value, valstruct *valp);
	extern vtw_node * make_node(vtw_oper_e oper, vtw_node *left,
	                            vtw_node *right);
	extern vtw_node *make_str_node(char *str);
	extern vtw_node *make_var_node(char *str);
	extern vtw_node *make_str_node0(char *str, vtw_oper_e op);
	extern void append(vtw_list *l, vtw_node *n, int aux);

	extern int yy_cli_val_lex(void);
	extern int parse_def(vtw_def *defp, const char *path, boolean type_only);

	extern vtw_path m_path, t_path;

	/*************************************************
	     GLOBAL FUNCTIONS
	***************************************************/
	extern void add_val(valstruct *first, valstruct *second);
	extern int cli_val_read(char *buf, int max_size);
	extern vtw_node *make_val_node(valstruct *val);
	extern valstruct str2val(char *cp);
	extern void free_val(valstruct *val);
	extern void my_malloc_init(void);

#define    VTWERR_BADPATH  -2
#define    VTWERR_OK     0

	extern FILE *err_stream;

	/* debug hooks for memory allocations */
#define CLI_MEM_DEBUG
#undef CLI_MEM_DEBUG
#ifdef CLI_MEM_DEBUG
#include <syslog.h>
static inline void *my_malloc(size_t size, const char *name)
{
	void *p = malloc(size);
	syslog(LOG_WARNING, "%s: %s (%lu bytes @ %p)", __func__, name, size, p);
	return p;
}

static inline void *my_realloc(void *ptr, size_t size, const char *name)
{
	void *p = realloc(ptr, size);
	syslog(LOG_WARNING, "%s: %s (%lu bytes @ %p)", __func__, name, size, p);
	return p;
}

static inline void my_free(void *ptr)
{
	syslog(LOG_WARNING, "%s: %p", __func__, ptr);
	free(ptr);
}

static inline char *my_strdup(const char *str, const char *name)
{
	char *s = strdup(str);
	syslog(LOG_WARNING, "%s: %s (%p=%s)", __func__, name, s, s);
	return s;
}
#else
#define my_malloc(size, name)		malloc(size)
#define my_realloc(ptr, size, name)	realloc(ptr, size)
#define my_free(ptr)			free(ptr)
#define my_strdup(str, name)		strdup(str)
#endif

	/*** debug ***/
#undef CLI_DEBUG
#ifdef CLI_DEBUG
#define DPRINT(fmt, ...)	printf(fmt, __VA_ARGS__)
#else
#define DPRINT(fmt, ...)	while (0) { printf(fmt, __VA_ARGS__); }
#endif
#ifdef __cplusplus
}
#endif

#endif
