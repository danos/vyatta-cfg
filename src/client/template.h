/*
 * Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
 *
   Copyright (c) 2013-2015 by Brocade Communications Systems, Inc.
   All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 */

#ifndef CONFIGD_TEMPLATE_H_
#define CONFIGD_TEMPLATE_H_

#ifdef __cplusplus
extern "C" {
#endif

struct configd_conn;
struct vector;
struct map;
struct configd_error;

char *configd_schema_get(struct configd_conn *, const char *module,
			 const char *fmt, struct configd_error *error);
char *configd_get_schemas(struct configd_conn *, struct configd_error *error);
/**
 * configd_get_features returns a map of schema ids and the
 * corresponding enabled features.
 *
 * On error the returned pointer is NULL, and if the error struct
 * pointer is non NULL the error will be populated.
 */
struct map *configd_get_features(struct configd_conn *, struct configd_error *error);
/**
 * configd_tmpl_get takes a '/' separated path and returns a map consisting of
 * a subset of its template definition.
 * The subset consists of:
 *	is_value: whether the node is a value
 *	type: the type of the node
 *	type2: the secondary type of the node
 *	help: the help text string
 *	multi: whether the node is a multi
 *	tag: whether the node is a tag
 *	limit: the limit of nodes for a tag node
 *	default: the default value for the node
 *	allowed: the allowed field
 *	val_help: the val_help field
 *	comp_help: the comp_help field
 *	secret: whether this node should be treated as sensitive information
 * On error the returned pointer is NULL, and if the error struct pointer is
 * non NULL the error will be populated.
 *
 * NOTE: this api is considered to be unstable and may change as we continue the resturcturing work.
 */
struct map *configd_tmpl_get(struct configd_conn *, const char *, struct configd_error *error);

/**
 * configd_tmpl_get_children takes a '/' spearated path and returns a vector
 * containing its children. On error the returned pointer is NULL, and if
 * the error struct pointer is non NULL the error will be populated.
 *
 * NOTE: these are the children of the template and not the values in the database.
 */
struct vector *configd_tmpl_get_children(struct configd_conn *, const char *, struct configd_error *error);

/**
 * configd_tmpl_get_allowed takes a '/' separated path and returns a vector
 * containing the allowed fields for that node. On error the returned pointer
 * is NULL, and if the error struct pointer is non NULL the error will be populated.
 *
 * NOTE: this evaluates the allowed script to generate the vector of allowed values.
 */
struct vector *configd_tmpl_get_allowed(struct configd_conn *, const char *, struct configd_error *error);


/**
 * configd_tmpl_validate_path takes a '/' separated path and return whether
 * it has a valid template. The return values are 0:false, 1:true, -1:error. On
 * error if the error struct pointer is non NULL the error will be populated.
 */
int configd_tmpl_validate_path(struct configd_conn *, const char *, struct configd_error *error);


/**
 * configd_tmpl_validate_path takes a '/' separated path and return whether it
 * has a valid template and the values are valid according to the schema. The
 * return values are 0:false, 1:true, -1:error. On error if the error struct
 * pointer is non NULL the error will be populated.
 */
int configd_tmpl_validate_values(struct configd_conn *, const char *, struct configd_error *error);

#ifdef __cplusplus
}
#endif


#endif
