/*
 * Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
 *
  Copyright (c) 2013, 2015 by Brocade Communications Systems, Inc.

   All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
*/

#ifndef CONFIGD_NODE_H_
#define CONFIGD_NODE_H_

#ifdef __cplusplus
extern "C" {
#endif

struct configd_error;
struct configd_conn;
struct vector;

#define CONFIGD_TREEGET_DEFAULTS (1 << 0)
#define CONFIGD_TREEGET_SECRETS  (1 << 1)
#define CONFIGD_TREEGET_ALL (CONFIGD_TREEGET_DEFAULTS|CONFIGD_TREEGET_SECRETS)

/**
 * configd_node_exists takes a '/' separated path and a database.
 * It returns whether the node at the path is in the database.
 * The return values are 0: false, 1: true, -1: error.
 * On error if the error struct pointer is non NULL the error will be filled out.
 */
int configd_node_exists(struct configd_conn *, int DB, const char *path, struct configd_error *);

/**
 * configd_node_is_default takes a '/' separated path and a database.
 * It returns whether the node at the path in the database is marked as default.
 * The return values are 0: false, 1: true, -1: error.
 * On error if the error struct pointer is non NULL the error will be filled out.
 */
int configd_node_is_default(struct configd_conn *, int DB, const char *path, struct configd_error *);

/**
 * configd_node_get takes a '/' separated path and a database. It returns a vector
 * consisting of the nodes value or values. Contrary to previous versions of this
 * API this function works on any node type. On error the pointer to the vector
 * will be NULL and if the error struct pointer is non NULL the error will be filled out.
 */
struct vector *configd_node_get(struct configd_conn *, int DB, const char *path, struct configd_error *);

/**
 * configd_node_get_status takes a '/' separated path and a database. It returns
 * the whether the node was 'added', 'deleted', 'changed', or 'unchanged' in
 * this database. The return values are one of the NodeStatus type or -1 on error.
 * On error if the error struct pointer is non NULL the error will be filled out.
 */
int configd_node_get_status(struct configd_conn *, int, const char *, struct configd_error *);

/**
 * configd_node_get_type takes a '/' separated path and a database. It returns
 * the node's type in the tree: 'leaf', 'multi', 'container', or 'tag'. The
 * return values are one of the NodeType type or -1 on error. On error if the
 * error struct pointer is non NULL the error will be filled out.
 */
int configd_node_get_type(struct configd_conn *, const char *, struct configd_error *);

/**
 * configd_node_get_comment takes a '/' separated path and a database. It returns
 * whether the node's comment if one exists or "" otherwise. On error the returned
 * pointer is NULL and if the error struct pointer is non NULL the error will be
 * filled out.
 */
char *configd_node_get_comment(struct configd_conn *, int, const char *, struct configd_error *);

/**
 * configd_tree_get_encoding takes a '/' separated path and a database. It returns an "encoding"
 * encoded string representing the configuration (sub)tree requested. On error
 * the pointer returned will be NULL and if the error struct pointer is non NULL
 * the error will be filled out.
 */
char *configd_tree_get_encoding(struct configd_conn *conn, int db, const char *path, const char *encoding, struct configd_error *error);

/**
 * configd_tree_get_encoding_flags takes a '/' separated path and a
 * database. It returns an "encoding" encoded string representing the
 * configuration (sub)tree requested containing content based on the
 * specified flags. On error the pointer returned will be NULL and if
 * the error struct pointer is non NULL the error will be filled out.
 */
char *configd_tree_get_encoding_flags(struct configd_conn *conn, int db, const char *path, const char *encoding, unsigned int flags, struct configd_error *error);

/**
 * configd_tree_get takes a '/' separated path and a database. It returns a JSON
 * encoded string representing the configuration (sub)tree requested. On error
 * the pointer returned will be NULL and if the error struct pointer is non NULL
 * the error will be filled out.
 */
char *configd_tree_get(struct configd_conn *, int, const char *, struct configd_error *);

/**
 * configd_tree_get_xml takes a '/' separated path and a database. It returns a
 * XML encoded string representing the configuration (sub)tree requested.
 * On error the pointer returned will be NULL and if the error struct pointer
 * is non NULL the error will be filled out.
 */
char *configd_tree_get_xml(struct configd_conn *, int, const char *, struct configd_error *);

/**
 * configd_tree_get_internal takes a '/' separated path and a database.
 * It returns a JSON encoded string representing the configuration (sub)tree
 * requested. This is encoded such that it will be easy to use from a
 * programmer's point of view. On error the pointer returned will be NULL and
 * if the error struct pointer is non NULL the error will be filled out.
 */
char *configd_tree_get_internal(struct configd_conn *, int, const char *, struct configd_error *);


/**
 * configd_tree_get_full_encoding takes a '/' separated path and a database. It returns an "encoding"
 * encoded string representing the configuration (sub)tree requested. On error
 * the pointer returned will be NULL and if the error struct pointer is non NULL
 * the error will be filled out.
 */
char *configd_tree_get_full_encoding(struct configd_conn *conn, int db, const char *path, const char *encoding, struct configd_error *error);

/**
 * configd_tree_get_full_encoding_flags takes a '/' separated path and
 * a database. It returns an "encoding" encoded string representing
 * the configuration (sub)tree requested containing content based on
 * the specified flags. On error the pointer returned will be NULL and
 * if the error struct pointer is non NULL the error will be filled
 * out.
 */
char *configd_tree_get_full_encoding_flags(struct configd_conn *conn, int db, const char *path, const char *encoding, unsigned int flags, struct configd_error *error);

/**
 * configd_tree_get_full takes a '/' separated path and a database. It returns
 * a JSON encoded string representing the (sub)tree requested. On error the
 * pointer returned will be NULL and if the error struct pointer is non NULL
 * the error will be filled out.
 */
char *configd_tree_get_full(struct configd_conn *, int, const char *, struct configd_error *);

/**
 * configd_tree_get_full_xml takes a '/' separated path and a database.
 * It returns a XML encoded string representing the (sub)tree requested.
 * On error the pointer returned will be NULL and if the error struct pointer
 * is non NULL the error will be filled out.
 */
char *configd_tree_get_full_xml(struct configd_conn *, int, const char *, struct configd_error *);

/**
 * configd_tree_get_full_internal takes a '/' separated path and a database.
 * It returns a JSON encoded string representing the (sub)tree requested.
 * This is encoded such that it will be easy to use from a programmer's point
 * of view. On error the pointer returned will be NULL and if the error struct
 * pointer is non NULL the error will be filled out.
 */
char *configd_tree_get_full_internal(struct configd_conn *, int, const char *, struct configd_error *);

/**
 * configd_node_get_complete_env returns environement variables used by the completion
 * scripts. The return values are 0:false, 1:true, -1:error. On error if the error struct
 * pointer is non NULL then the error will be populated.
 */
char *configd_node_get_complete_env(struct configd_conn *, const char *, struct configd_error *);

#ifdef __cplusplus
}
#endif

#endif
