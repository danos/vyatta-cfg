/*
 * Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
 *
  Copyright (c) 2013-2014 by Brocade Communications Systems, Inc.

   All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
*/

#ifndef CONFIGD_SESSION_H_
#define CONFIGD_SESSION_H_

#ifdef __cplusplus
extern "C" {
#endif

struct configd_conn;
struct configd_error;

/**
 * configd_sess_exists returns whether the session id exists. The return values
 * are 0:false, 1:true, -1:error. On error if the error struct pointer is non NULL
 * then the error will be populated.
 */
int configd_sess_exists(struct configd_conn *, struct configd_error *);


/**
 * configd_sess_setup creates a new configuration session and corresponding
 * candidate database. The return values are 0:success, -1:error. On error
 * if the error struct pointer is non NULL then the error will be populated.
 */
int configd_sess_setup(struct configd_conn *, struct configd_error *);


/**
 * configd_sess_teardown destroys the configuration session and corresponding
 * candidate database. The return values are 0:success, -1:error. On error
 * if the error struct pointer is non NULL then the error will be populated.
 */
int configd_sess_teardown(struct configd_conn *, struct configd_error *);

/**
 * configd_sess_lock locks a shared candidate database. 
 * The return values are 0:success, -1:error. On error
 * if the error struct pointer is non NULL then the error will be populated.
 */
int configd_sess_lock(struct configd_conn *conn, struct configd_error *error);

/**
 * configd_sess_unlock unlocks a shared candidate database. 
 * The return values are 0:success, -1:error. On error
 * if the error struct pointer is non NULL then the error will be populated.
 */
int configd_sess_unlock(struct configd_conn *conn, struct configd_error *error);

/**
 * configd_sess_locked returns whether the session is currently locked
 * The return values are >0:locked by pid 0:unlocked, -1:error. On error
 * if the error struct pointer is non NULL then the error will be populated.
 */
int configd_sess_locked(struct configd_conn *conn, struct configd_error *error);

/**
 * configd_sess_changed returns whether the candidate database for this session
 * id has changes. The return values are 0:false, 1:true, -1:error. On error
 * if the error struct pointer is non NULL then the error will be populated.
 */
int configd_sess_changed(struct configd_conn *, struct configd_error *);


/**
 * configd_sess_saved reports whether the candidate database for this session
 * id has been saved. The return values are 0:false, 1:true, -1:error. On
 * error if the error struct pointer is non NULL then the error will be populated.
 */
int configd_sess_saved(struct configd_conn *, struct configd_error *);


/**
 * configd_sess_mark_saved marks the database as saved. The return values
 * are 0:success, -1:error. On error if the error struct pointer is non
 * NULL then the error will be populated.
 */
int configd_sess_mark_saved(struct configd_conn *, struct configd_error *);


/**
 * configd_sess_mark_unsaved marks the database as unsaved. The return values
 * are 0:success, -1:error. On error if the error struct pointer is non NULL
 * then the error will be populated.
 */
int configd_sess_mark_unsaved(struct configd_conn *, struct configd_error *);


/**
 * configd_sess_get_env returns the session environment variables that the shell
 * needs in config mode. On error the returned pointer is NULL, and if the error
 * struct pointer is non NULL then the error will be populated.
 */
char *configd_sess_get_env(struct configd_conn *, struct configd_error *);


/**
 * configd_edit_get_env returns the environment variables for the new 'edit mode'
 * that is used by the config mode shell. On error the returned pointer is NULL,
 * and if the error struct pointer is non NULL then the error will be populated.
 */
char *configd_edit_get_env(struct configd_conn *, const char *, struct configd_error *);

/**
 * configd_get_help returns the a map consisting of the next possible parts of
 * a command and its help text. On error the returned pointer is NULL,
 * and if the error struct pointer is non NULL then the error will be populated.
 */
struct map *configd_get_help(struct configd_conn *conn, int from_schema, const char *cpath, struct configd_error *error);

#ifdef __cplusplus
}
#endif

#endif
