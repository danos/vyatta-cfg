/*
 * Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
 *
 * Copyright (c) 2015 by Brocade Communications Systems, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 */

//for more information about this wrapper see SWIG documentation:
//http://www.swig.org/Doc2.0/Ruby.html#Ruby
%module "vyatta::configd"

%rename("%(undercase)s", notregexmatch$name="CfgClient*") "";
%rename("%s", regexmatch$name="^[A-Z]+$") ""; //don't rename constants
%rename("Client") CfgClient;
%rename("FatalException") CfgClientFatalException;
%rename("Exception") CfgClientException;

%feature("autodoc", "2");
%typemap(doc) std::vector<std::string> const & path "$1_name: Space separated string or Sequence of strings representing the configuration path";

%ignore string2vec;

%typemap(in) std::vector<std::string> & (std::vector<std::string> vec) {
	switch (TYPE($input)) {
		case T_STRING:
			{
				std::string str = StringValueCStr($input);
				string2vec(str, vec);
			}
			break;
		case T_ARRAY:
			{
				int len = RARRAY_LEN($input);
				for (int i = 0; i < len; i++) {
					VALUE v = rb_ary_entry($input, i);
					std::string str = StringValuePtr(v);
					vec.push_back(str);
				}
			}
			break;
		default:
			break;
	}
	$1 = &vec;
}

%typemap(out) std::vector<std::string> {
	std::vector<std::string> &vec = $1;
	VALUE out = rb_ary_new2(vec.size());
	for (unsigned int i = 0; i < vec.size(); i++) {
		rb_ary_store(out, i, rb_str_new2(vec[i].c_str()));
	}
	$result = out;
}

%typemap(out) std::map<std::string, std::string> {
	std::map<std::string, std::string> &m = $1;
	VALUE out = rb_hash_new();
	std::map<std::string, std::string>::iterator it = m.begin();
	for (; it != m.end(); ++it) {
		VALUE key = rb_str_new2((*it).first.c_str());
		VALUE val = rb_str_new2((*it).second.c_str());
		rb_hash_aset(out, key, val);
	}
	$result = out;
}

%typemap(out) std::map<std::string, std::vector<std::string> > {
	std::map<std::string, std::vector<std::string> > &m = $1;
	VALUE out = rb_hash_new();
	std::map<std::string, std::vector<std::string> >::iterator it = m.begin();
	for (; it != m.end(); ++it) {
		VALUE key = rb_str_new2((*it).first.c_str());
		std::vector<std::string> &vec = (*it).second;
		VALUE val = rb_ary_new2(vec.size());
		for (unsigned int i = 0; i < vec.size(); i++) {
			rb_ary_store(val, i, rb_str_new2(vec[i].c_str()));
		}
		rb_hash_aset(out, key, val);
	}
	$result = out;
}

%{
#define SWIG_FILE_WITH_INIT
#include "../CfgClient.hpp"
VALUE rb_json_load(const std::string &str) {
	rb_require("json");
	VALUE mod = rb_const_get(rb_cModule, rb_intern("JSON"));
	return rb_funcall(mod, rb_intern("load"), 1, rb_str_new2(str.c_str()));
}
std::string rb_json_dump(VALUE obj) {
	rb_require("json");
	VALUE mod = rb_const_get(rb_cModule, rb_intern("JSON"));
	VALUE output = rb_funcall(mod, rb_intern("dump"), 1, obj);
	std::string out = StringValueCStr(output);
	return out;
}
%}

%include "std_string.i";

%include "../CfgClient.hpp"

%extend CfgClientException {
	std::string message() {
		return $self->what();
	}
	std::string to_s() {
		return $self->what();
	}
	std::string inspect() {
		return $self->what();
	}
}

%extend CfgClientFatalException {
	std::string message() {
		return $self->what();
	}
	std::string to_s() {
		return $self->what();
	}
	std::string inspect() {
		return $self->what();
	}
}

%extend CfgClient {
	std::vector<std::string> get(const std::vector<std::string> &path) {
		return $self->NodeGet($self->AUTO, path);
	}
	VALUE tree_get_hash(const std::vector<std::string> &path) {
		return rb_json_load($self->TreeGetInternal($self->AUTO, path));
	}
	VALUE tree_get_full_hash(const std::vector<std::string> &path) {
		return rb_json_load($self->TreeGetFullInternal($self->AUTO, path));
	}
	VALUE call_rpc_hash(std::string ns, std::string name, VALUE input) {
		std::string inputStr = rb_json_dump(input);
		std::string output = $self->CallRPC(ns, name, inputStr);
		return rb_json_load(output);
	}
}
