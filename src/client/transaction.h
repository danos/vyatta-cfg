/*
 * Copyright (c) 2019-2021, AT&T Intellectual Property. All rights reserved.
 *
  Copyright (c) 2013-2016 by Brocade Communications Systems, Inc.

   All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
*/

#ifndef CONFIGD_TRANSACTION_H_
#define CONFIGD_TRANSACTION_H_

#ifdef __cplusplus
extern "C" {
#endif

struct configd_conn;
struct configd_error;

/**
 * configd_show is a mess, do not rely on it for API calls it is used only in
 * cli-shell-api.
 */
char *configd_show(struct configd_conn *, int, const char *, const char *, const char *, int, struct configd_error *);

/**
 * configd_set takes a '/' separated path and attemptes to create it in the
 * candidate database. On error the pointer it returns is set to NULL and
 * if the configd_error struct is non NULL the error is populated.
 */
char *configd_set(struct configd_conn *, const char *, struct configd_error *);

/**
 * configd_validate_path takes a '/' separated path and verifies the syntax of the path
 * On error the pointer it returns is set to NULL and if the configd_error struct is
 * non NULL the error is populated.
 */
char *configd_validate_path(struct configd_conn *, const char *, struct configd_error *);

/**
 * configd_delete takes a '/' separated path and attemptes to delete it from
 * the candidate database. On error the pointer it returns is set to NULL and
 * if the configd_error struct is non NULL the error is populated.
 */
char *configd_delete(struct configd_conn *, const char *, struct configd_error *);

/**
 * configd_rename takes 2 '/' separated paths and attemptes to rename the first
 * path to the second. On error the pointer it returns is set to NULL and if the
 * configd_error struct is non NULL the error is populated
 */
char *configd_rename(struct configd_conn *, const char *, struct configd_error *);


/**
 * configd_copy takes 2 '/' separated paths and attemptes to copy the first path
 * to the second. On error the pointer it returns is set to NULL and if the
 * configd_error struct is non NULL the error is populated
 */
char *configd_copy(struct configd_conn *, const char *, struct configd_error *);


/**
 * configd_comment takes a '/' separated path and attemptes to create the comment
 * in the candidate database. On error the pointer it returns is set to NULL and
 * if the configd_error struct is non NULL the error is populated.
 */
char *configd_comment(struct configd_conn *, const char *, struct configd_error *);


/**
 * configd_commit preforms the commit operation on the candidate database. On error
 * the pointer it returns is set to NULL and if the configd_error struct is non NULL
 * the error is populated.
 */
char *configd_commit(struct configd_conn *, const char *, struct configd_error *);

/**
 * configd_confirmed_commit preforms the commit operation on the candidate database. On error
 * the pointer it returns is set to NULL and if the configd_error struct is non NULL
 * the error is populated.
 */
char *configd_confirmed_commit(struct configd_conn *, const char *, int, const char *, const char *, const char *, struct configd_error *);

/**
 * configd_cancel_commit reverts a pending confirmed commit operation. On error
 * the pointer it returns is set to NULL and if the configd_error struct is non NULL
 * the error is populated.
 */
char *configd_cancel_commit(struct configd_conn *, const char *, const char *, struct configd_error *);

/**
 * configd_discard destroys all pending changes in the candidate database. On error
 * the pointer it returns is set to NULL and if the configd_error struct is non NULL the
 * error is populated.
 */
char *configd_discard(struct configd_conn *, struct configd_error *);


/**
 * configd_save takes a filesystem path and writes the configuration to a file at
 * that location. On error the pointer it returns is set to NULL and if the
 * configd_error struct is non NULL the error is populated.
 */
char *configd_save(struct configd_conn *, const char *, struct configd_error *);


/**
 * configd_load takes a filesystem path and attempts to read the configuration at
 * that path. On error the pointer it returns is set to NULL and if the
 * configd_error struct is non NULL the error is populated
 */
int configd_load(struct configd_conn *, const char *, struct configd_error *);

/**
 * configd_merge takes a filesystem path and attempts to read configuration segments
 * from that path and adds/replaces these segments to the candidate configuration.
 * On error the pointer it returns is set to NULL and if the configd_error struct
 * is non NULL the error is populated
 */
int configd_merge(struct configd_conn *, const char *, struct configd_error *);

/**
 * configd_load_report_warnings takes a filesystem path and attempts to read
 * the configuration at that path. On error the pointer it returns is set
 * to NULL and if the configd_error struct is non NULL the error is populated.
 * Additionally, if some line(s) of configuration generate warnings, but no
 * error is reported, a valid pointer is returned along with the error struct
 * populated if provided.
 */
int configd_load_report_warnings(struct configd_conn *, const char *, struct configd_error *);


/**
 * configd_validate preforms the validate operation on the candidate database. On
 * error the pointer it returns is set to NULL and if the configd_error struct is
 * non NULL the error is populated.
 */
char *configd_validate(struct configd_conn *, struct configd_error *);

/**
 * configd_validate_config preforms the validate operation the supplied config. On
 * error the pointer it returns is set to NULL and if the configd_error struct is
 * non NULL the error is populated.
 */
char *configd_validate_config(struct configd_conn *, const char *encoding, const char *config, struct configd_error *);

/**
 * configd_edit_config_xml preforms a RFC-6241 edit-config operation.
 * On error the pointer it returns is set to NULL and if the
 * configd_error struct is non NULL the error is populated.
 */
char *configd_edit_config_xml(struct configd_conn *conn,
			      const char *config_target,
			      const char *default_operation,
			      const char *test_option,
			      const char *error_option,
			      const char *config,
			      struct configd_error *error);

/**
 * configd_copy_config performs a RFC-6241 copy-config operation.
 * On error the pointer it returns is set to NULL and if the
 * configd_error struct is non NULL the error is populated.
 */
char *configd_copy_config(
	struct configd_conn *conn,
	const char *source_datastore,
	const char *source_encoding,
	const char *source_config,
	const char *source_url,
	const char *target_datastore,
	const char *target_url,
	struct configd_error *error);

#ifdef __cplusplus
}
#endif

#endif
