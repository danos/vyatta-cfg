/*
 * Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
 *
 * Copyright (c) 2014-2015 by Brocade Communications Systems, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 */
%{
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>

#include "cli_val.h"

extern int yy_cli_def_lineno;
extern char *yy_cli_def_text;
static vtw_def *parse_defp;
static int parse_status; 
static boolean cli_def_type_only=0; /*{ if (cli_def_type_only) YYACCEPT;}*/
/* XXX: sigh, the -p flag to yacc should do this for us kkkk*/
#define yystacksize tpltstacksize
#define yysslim tpltsslim

/* forward prototypes */
extern int yy_cli_parse_parse();
extern int yy_cli_def_lex();
extern void yy_cli_parse_error(const char *);
static  void cli_deferror(const char *);
 extern FILE *yy_cli_def_in;
#define YYDEBUG 1
#define yy_cli_parse_lex yy_cli_def_lex
%}
%token EOL
%token MULTI
%token TAG
%token TYPE
%token HELP
%token DEFAULT
%token PRIORITY
%token SECRET
%token ENUMERATION
%token CHELP
%token ALLOWED
%token VHELP
%token PATTERN
%token EXEC
%token SYNTAX
%token SUB
%token COMMIT
%token CHECK
%token DUMMY
%left SEMI
%token <val>VALUE
%token <type>TYPE_DEF
%token <strp>VAR
%token <strp> STRING
%token <strp> EX_STRING
%token SYNTAX_ERROR
%token <action>ACTION
%left ASSIGN
%left OR
%left AND
%right NOT
%left COMMA
%nonassoc <cond>COND
%token RP
%token LP
%type <nodep> val
%type <nodep> exp
%type <nodep> assign_exp
%type <val>   val0
%type <nodep> action
%type <nodep> action0

%union {
  char *strp;
  valstruct val;
  vtw_type_e type;
  vtw_cond_e cond;
  vtw_node *nodep;
  vtw_act_type action;
}

%%

input:          tag 
                | EOL input
                | tag otherinput
		;

otherinput:     type EOL 
                | cause EOL
                | otherinput type EOL 
                | otherinput cause EOL
                | otherinput EOL
                | EOL
		| syntax_error 
		;

tag:            /* empty */
                | TAG EOL {parse_defp->tag = TRUE;}
                | TAG VALUE {
		    parse_defp->tag = TRUE;
		    char *tmp = $2.val;
		    long long int cval = 0;
		    char *endp = NULL;
		    errno = 0;
		    cval = strtoll(tmp, &endp, 10);
		    if (($2.val_type != INT_TYPE)
			|| (errno == ERANGE
			    && (cval == LLONG_MAX || cval == LLONG_MIN))
			|| (errno != 0 && cval == 0)
			|| (*endp != '\0') || (cval < 0) || (cval > UINT_MAX)) {
		      yy_cli_parse_error((const char *)
					 "Tag must be <u32>\n");
		    } else {
		      parse_defp->def_tag = cval;
		    }
                  }
		| MULTI EOL {parse_defp->multi = TRUE;}
		| MULTI VALUE 
		{
  		    parse_defp->multi = TRUE;
		    char *tmp = $2.val;
		    long long int cval = 0;
		    char *endp = NULL;
		    errno = 0;
		    cval = strtoll(tmp, &endp, 10);
		    if (($2.val_type != INT_TYPE)
			|| (errno == ERANGE
			    && (cval == LLONG_MAX || cval == LLONG_MIN))
			|| (errno != 0 && cval == 0)
			|| (*endp != '\0') || (cval < 0) || (cval > UINT_MAX)) {
		      yy_cli_parse_error((const char *)
					 "Tag must be <u32>\n");
		    } else {
		      parse_defp->def_multi = cval;
		    }
                  }
		;
type:           TYPE TYPE_DEF COMMA TYPE_DEF
                {
                  parse_defp->def_type = $2;
                  parse_defp->def_type2 = $4;
                }
                ;

type:	      	TYPE TYPE_DEF SEMI STRING
		{ parse_defp->def_type = $2; 
                  parse_defp->def_type_help = $4; }
		;


type:	      	TYPE TYPE_DEF
		{ parse_defp->def_type = $2; 
                }
		;

cause:		help_cause
		| default_cause
		| priority_stmt
		| secret_stmt
                | enumeration_stmt
                | chelp_stmt
                | allowed_stmt
                | vhelp_stmt
		| syntax_cause
                | ACTION action { append(parse_defp->actions + $1, $2, 0);}
                | dummy_stmt
		;

dummy_stmt:     DUMMY STRING { /* ignored */ }
                ;

help_cause:	HELP STRING 
                { parse_defp->def_node_help = $2; /* no semantics for now */ 
                }

default_cause:  DEFAULT VALUE 
		{
		   if ($2.val_type != parse_defp->def_type)
		     yy_cli_parse_error((const char *)"Bad default\n");
		   parse_defp->def_default = $2.val;
		}
default_cause:  DEFAULT STRING 
		{
		   if (TEXT_TYPE != parse_defp->def_type)
		     yy_cli_parse_error((const char *)"Bad default\n");
		   parse_defp->def_default = $2;
		}
priority_stmt:  PRIORITY VALUE
                {
                  char *tmp = $2.val;
		  long long int cval = 0;
                  char *endp = NULL;
                  errno = 0;
                  cval = strtoll(tmp, &endp, 10);
                  if ((errno == ERANGE
                          && (cval == LLONG_MAX || cval == LLONG_MIN))
                      || (errno != 0 && cval == 0)
                      || (*endp != '\0') || (cval < 0) || (cval > UINT_MAX)) {
		    parse_defp->def_priority_ext = tmp;
                  } else {
                    parse_defp->def_priority = cval;
                  }
                }

secret_stmt:	SECRET
		{
			parse_defp->secret = TRUE;
		}

enumeration_stmt: ENUMERATION STRING
                  {
                    parse_defp->def_enumeration = $2;
                  }

chelp_stmt: CHELP STRING
            {
              parse_defp->def_comp_help = $2;
            }

allowed_stmt: ALLOWED STRING
              {
                parse_defp->def_allowed = $2;
              }

vhelp_stmt: VHELP STRING
            {
              if (!(parse_defp->def_val_help)) {
                /* first string */
                parse_defp->def_val_help = $2;
              } else {
                /* subsequent strings */
                char *optr = parse_defp->def_val_help;
                int olen = strlen(parse_defp->def_val_help);
                char *nptr = $2;
                int nlen = strlen(nptr);
                int len = olen + 1 /* "\n" */ + nlen + 1 /* 0 */;
                char *mptr = (char *) malloc(len);
                if (mptr) {
                  memcpy(mptr, optr, olen);
                  mptr[olen] = '\n';
                  memcpy(&(mptr[olen + 1]), nptr, nlen);
                  mptr[len - 1] = 0;
                }
                parse_defp->def_val_help = mptr;
                free(optr);
                free(nptr);
              }
              /* result is a '\n'-delimited string for val_help */
            }

syntax_cause:   SYNTAX action {append(parse_defp->actions + syntax_act, $2, 0);}
		| SUB assign_exp {append(parse_defp->actions + sub_act, $2, 0);}
		| COMMIT action {append(parse_defp->actions + commit_act, $2, 1);}
		;

action0:        STRING { $$ = make_node(EXEC_OP, make_str_node($1),NULL);}
                ;
action:         action0
                | action0 SEMI STRING 
                   {$$ = make_node(HELP_OP, $1, make_str_node($3));}
                | exp 
                ;
exp:		  LP exp RP 
		   {$$=$2;}
		| exp AND exp {$$ = make_node(AND_OP,$1,$3);}
		| exp OR exp {$$ = make_node(OR_OP,$1,$3);}
		| val COND val 
		   {$$ = make_node(COND_OP,$1,$3);$$->vtw_node_aux = $2;}
		| PATTERN VAR STRING 
		   { $$ = make_node(PATTERN_OP,make_var_node($2),
				     make_str_node($3));}
		| EXEC STRING
		   { $$ = make_node(EXEC_OP,make_str_node($2),NULL);}
		| NOT exp {$$ = make_node(NOT_OP,$2,NULL);}
                | exp SEMI STRING 
                   {$$ = make_node(HELP_OP, $1, make_str_node($3));}
                | VAR ASSIGN val
                   {$$ = make_node(ASSIGN_OP, make_var_node($1), $3);}
                ;

assign_exp:	VAR ASSIGN val
                   {$$ = make_node(ASSIGN_OP, make_var_node($1), $3);}
		| assign_exp AND assign_exp
		   {$$ = make_node(AND_OP,$1,$3);}
		| assign_exp OR assign_exp
		   {$$ = make_node(OR_OP,$1,$3);}
		| assign_exp SEMI STRING 
                   {$$ = make_node(HELP_OP, $1, make_str_node($3));}
		;

val:		  VAR {$$ = make_var_node($1);}
                | val0 {$$ = make_val_node(&($1));}
                | EX_STRING {$$=make_str_node0($1, B_QUOTE_OP);}
		;

val0:           VALUE 

                | val0 COMMA val0 { add_val(&($1), &($3)); $$=$1; }

		| STRING {$$ = str2val($1);}
                ;

syntax_error:	  SYNTAX_ERROR {
			cli_deferror("syntax error");
		}
		;


%%
const char *parse_path;
int parse_def(vtw_def *defp, const char *path, boolean type_only)
{
   int status;
   /* always zero vtw_def struct */
   memset(defp, 0, sizeof(vtw_def));
   yy_cli_def_lineno = 1;
   parse_status = 0;
   parse_defp = defp;
   cli_def_type_only = type_only;
   yy_cli_def_in = fopen(path, "r");
#if 0
   yy_cli_parse_debug = 1;
#endif
   if (!yy_cli_def_in)
     return -5;
   parse_path = path;
   status = yy_cli_parse_parse(); /* 0 is OK */
   fclose(yy_cli_def_in);
   return status;
}
static void
cli_deferror(const char *s)
{
  printf("Error: %s in file [%s], line %d, last token [%s]\n",s, parse_path, 
	 yy_cli_def_lineno, yy_cli_def_text);
}

void yy_cli_parse_error(const char *s)
{
  cli_deferror(s);
}
  
  



