/*
 * Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
 *
 * Copyright (C) 2010-2013 Vyatta, Inc. All rights reserved.
 *
 * Copyright (c) 2014 by Brocade Communications Systems, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 */

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

/* these macros are defined in perl headers but conflict with C++ headers */
#undef do_open
#undef do_close

#include <cstring>
#include <vector>
#include <string>
#include <map>

#include <compat/cstore-compat.hpp>

using namespace cstore;

typedef SV STRVEC;
typedef SV CPATH;
typedef SV STRSTRMAP;

MODULE = Cstore		PACKAGE = Cstore		


Cstore *
Cstore::new()
CODE:
  RETVAL = new Cstore();
OUTPUT:
  RETVAL


void
Cstore::DESTROY()

SV *
Cstore::cfgGetTree(CPATH *pref, bool active_config)
PREINIT:
  std::vector<std::string> arg_cpath;
CODE:
  std::string value;
  if (THIS->cfgGetTree(arg_cpath, value, active_config)) {
    RETVAL = newSVpv(value.c_str(), 0);
  } else {
    XSRETURN_UNDEF;
  }
OUTPUT:
  RETVAL

SV *
Cstore::cfgPathGetType(CPATH *pref)
PREINIT:
  std::vector<std::string> arg_cpath;
CODE:
  std::string value;
  if (THIS->cfgPathGetType(arg_cpath, value)) {
    RETVAL = newSVpv(value.c_str(), 0);
  } else {
    XSRETURN_UNDEF;
  }
OUTPUT:
  RETVAL

bool
Cstore::cfgPathExists(CPATH *pref, bool active_cfg)
PREINIT:
  std::vector<std::string> arg_cpath;
CODE:
  RETVAL = THIS->cfgPathExists(arg_cpath, active_cfg);
OUTPUT:
  RETVAL


bool
Cstore::cfgPathDefault(CPATH *pref, bool active_cfg)
PREINIT:
  std::vector<std::string> arg_cpath;
CODE:
  RETVAL = THIS->cfgPathDefault(arg_cpath, active_cfg);
OUTPUT:
  RETVAL


STRVEC *
Cstore::cfgPathGetChildNodes(CPATH *pref, bool active_cfg)
PREINIT:
  std::vector<std::string> arg_cpath;
CODE:
  std::vector<std::string> ret_strvec;
  THIS->cfgPathGetChildNodes(arg_cpath, ret_strvec, active_cfg);
OUTPUT:
  RETVAL


SV *
Cstore::cfgPathGetValue(CPATH *pref, bool active_cfg)
PREINIT:
  std::vector<std::string> arg_cpath;
CODE:
  std::string value;
  if (THIS->cfgPathGetValue(arg_cpath, value, active_cfg)) {
    RETVAL = newSVpv(value.c_str(), 0);
  } else {
    XSRETURN_UNDEF;
  }
OUTPUT:
  RETVAL


STRVEC *
Cstore::cfgPathGetValues(CPATH *pref, bool active_cfg)
PREINIT:
  std::vector<std::string> arg_cpath;
CODE:
  std::vector<std::string> ret_strvec;
  THIS->cfgPathGetValues(arg_cpath, ret_strvec, active_cfg);
OUTPUT:
  RETVAL


bool
Cstore::cfgPathEffective(CPATH *pref)
PREINIT:
  std::vector<std::string> arg_cpath;
CODE:
  RETVAL = THIS->cfgPathEffective(arg_cpath);
OUTPUT:
  RETVAL


STRVEC *
Cstore::cfgPathGetEffectiveChildNodes(CPATH *pref)
PREINIT:
  std::vector<std::string> arg_cpath;
CODE:
  std::vector<std::string> ret_strvec;
  THIS->cfgPathGetEffectiveChildNodes(arg_cpath, ret_strvec);
OUTPUT:
  RETVAL


SV *
Cstore::cfgPathGetEffectiveValue(CPATH *pref)
PREINIT:
  std::vector<std::string> arg_cpath;
CODE:
  std::string value;
  if (THIS->cfgPathGetEffectiveValue(arg_cpath, value)) {
    RETVAL = newSVpv(value.c_str(), 0);
  } else {
    XSRETURN_UNDEF;
  }
OUTPUT:
  RETVAL


STRVEC *
Cstore::cfgPathGetEffectiveValues(CPATH *pref)
PREINIT:
  std::vector<std::string> arg_cpath;
CODE:
  std::vector<std::string> ret_strvec;
  THIS->cfgPathGetEffectiveValues(arg_cpath, ret_strvec);
OUTPUT:
  RETVAL


bool
Cstore::cfgPathDeleted(CPATH *pref)
PREINIT:
  std::vector<std::string> arg_cpath;
CODE:
  RETVAL = THIS->cfgPathDeleted(arg_cpath);
OUTPUT:
  RETVAL


bool
Cstore::cfgPathAdded(CPATH *pref)
PREINIT:
  std::vector<std::string> arg_cpath;
CODE:
  RETVAL = THIS->cfgPathAdded(arg_cpath);
OUTPUT:
  RETVAL


bool
Cstore::cfgPathChanged(CPATH *pref)
PREINIT:
  std::vector<std::string> arg_cpath;
CODE:
  RETVAL = THIS->cfgPathChanged(arg_cpath);
OUTPUT:
  RETVAL


STRVEC *
Cstore::cfgPathGetDeletedChildNodes(CPATH *pref)
PREINIT:
  std::vector<std::string> arg_cpath;
CODE:
  std::vector<std::string> ret_strvec;
  THIS->cfgPathGetDeletedChildNodes(arg_cpath, ret_strvec);
OUTPUT:
  RETVAL


STRVEC *
Cstore::cfgPathGetDeletedValues(CPATH *pref)
PREINIT:
  std::vector<std::string> arg_cpath;
CODE:
  std::vector<std::string> ret_strvec;
  THIS->cfgPathGetDeletedValues(arg_cpath, ret_strvec);
OUTPUT:
  RETVAL


STRSTRMAP *
Cstore::cfgPathGetChildNodesStatus(CPATH *pref)
PREINIT:
  std::vector<std::string> arg_cpath;
CODE:
  std::map<std::string, std::string> ret_strstrmap;
  THIS->cfgPathGetChildNodesStatus(arg_cpath, ret_strstrmap);
OUTPUT:
  RETVAL


SV *
Cstore::cfgPathStatus(CPATH *pref)
PREINIT:
  std::vector<std::string> arg_cpath;
CODE:
  std::string value;
  if (THIS->cfgPathStatus(arg_cpath, value)) {
    RETVAL = newSVpv(value.c_str(), 0);
  } else {
    XSRETURN_UNDEF;
  }
OUTPUT:
  RETVAL

STRVEC *
Cstore::tmplGetChildNodes(CPATH *pref)
PREINIT:
  std::vector<std::string> arg_cpath;
CODE:
  std::vector<std::string> ret_strvec;
  THIS->tmplGetChildNodes(arg_cpath, ret_strvec);
OUTPUT:
  RETVAL


bool
Cstore::validateTmplPath(CPATH *pref, bool validate_vals)
PREINIT:
  std::vector<std::string> arg_cpath;
CODE:
  RETVAL = THIS->validateTmplPath(arg_cpath, validate_vals);
OUTPUT:
  RETVAL


STRSTRMAP *
Cstore::getParsedTmpl(CPATH *pref, bool allow_val)
PREINIT:
  std::vector<std::string> arg_cpath;
CODE:
  std::map<std::string, std::string> ret_strstrmap;
  if (!THIS->getParsedTmpl(arg_cpath, ret_strstrmap, allow_val)) {
    XSRETURN_UNDEF;
  }
OUTPUT:
  RETVAL


SV *
Cstore::cfgPathGetComment(CPATH *pref, bool active_cfg)
PREINIT:
  std::vector<std::string> arg_cpath;
CODE:
  std::string comment;
  if (THIS->cfgPathGetComment(arg_cpath, comment, active_cfg)) {
    RETVAL = newSVpv(comment.c_str(), 0);
  } else {
    XSRETURN_UNDEF;
  }
OUTPUT:
  RETVAL


bool
Cstore::sessionChanged()
CODE:
  RETVAL = THIS->sessionChanged();
OUTPUT:
  RETVAL


bool
Cstore::loadFile(char *filename)
CODE:
  RETVAL = THIS->loadFile(filename);
OUTPUT:
  RETVAL


bool
Cstore::inSession()
CODE:
  RETVAL = THIS->inSession();
OUTPUT:
  RETVAL
