/*
 * Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
 *
 * Copyright (c) 2014 by Brocade Communications Systems, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 */

#ifndef _CSTORE_C_H_
#define _CSTORE_C_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "../cli_cstore.h"

int cstore_set_cfg_path(const char *path);

/* the following are internal APIs for the library. they can only be used
 * during cstore operations since they operate on "current" paths constructed
 * by the operations.
 */
int cstore_get_var_ref(void *handle, const char *ref_str, vtw_type_e *type,
		       char **val, int *ismulti, int from_active);
int cstore_set_var_ref(void *handle, const char *ref_str, const char *value,
		       int to_active);

#ifdef __cplusplus
}
#endif
#endif /* _CSTORE_C_H_ */

