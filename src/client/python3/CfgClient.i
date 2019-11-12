/*
 * Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
 *
 * Copyright (c) 2015-2016 by Brocade Communications Systems, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 */
%module configd

%rename("%(undercase)s", notregexmatch$name="CfgClient*") "";
%rename("%s", regexmatch$name="^[A-Z_]+$") ""; //don't rename constants
%rename("Client") CfgClient;
%rename("FatalException") CfgClientFatalException;
%rename("Exception") CfgClientException;

%feature("autodoc", "2");
%typemap(doc) std::vector<std::string> const & path "$1_name: Space separated string or Sequence of strings representing the configuration path";

%ignore string2vec;

%typemap(in) std::vector<std::string> & (std::vector<std::string> vec) {
	if (!PySequence_Check($input)) {
		PyErr_SetString(PyExc_ValueError,"Expected a sequence");
		return NULL;
	}
	if (PyUnicode_Check($input)) {
		PyObject *str_obj = PyUnicode_AsUTF8String($input);
		std::string str = PyBytes_AsString(str_obj);
		Py_DECREF(str_obj);
		string2vec(str, vec);
		$1 = &vec;
	} else {
		int len = PySequence_Length($input);
		for (int i = 0; i < len; i++) {
			PyObject *o = PySequence_GetItem($input, i);
			if (PyUnicode_Check(o)) {
				PyObject *str_obj = PyUnicode_AsUTF8String(o);
				std::string str = PyBytes_AsString(str_obj);
				Py_DECREF(str_obj);
				vec.push_back(str);
			} else {
				o = PyObject_Str(o);
				if (o != NULL && PyUnicode_Check(o)) {
					PyObject *str_obj = PyUnicode_AsUTF8String(o);
					std::string str = PyBytes_AsString(str_obj);
					Py_DECREF(str_obj);
					vec.push_back(str);
				} else {
					PyErr_SetString(PyExc_ValueError, "Cannot convert value to string in the argument list");
					return NULL;
				}
			}
		}
	}
	$1 = &vec;
}

%typemap(out) std::vector<std::string> {
	std::vector<std::string> &vec = $1;
	PyObject *out = PyList_New(vec.size());
	for (unsigned int i = 0; i < vec.size(); i++) {
		PyList_SetItem(out, i, PyUnicode_FromString(vec[i].c_str()));
	}
	$result = out;
}

%typemap(out) std::map<std::string, std::string> {
	std::map<std::string, std::string> &m = $1;
	PyObject *out = PyDict_New();
	std::map<std::string, std::string>::iterator it = m.begin();
	for (; it != m.end(); ++it) {
		PyObject *key = PyUnicode_FromString((*it).first.c_str());
		PyObject *val = PyUnicode_FromString((*it).second.c_str());
		PyDict_SetItem(out, key, val);
	}
	$result = out;
}

%typemap(out) std::map<std::string, std::vector<std::string> > {
	std::map<std::string, std::vector<std::string> > &m = $1;
	PyObject *out = PyDict_New();
	std::map<std::string, std::vector<std::string> >::iterator it = m.begin();
	for (; it != m.end(); ++it) {
		PyObject *key = PyUnicode_FromString((*it).first.c_str());
		std::vector<std::string> &vec = (*it).second;
		PyObject *val = PyList_New(vec.size());
		for (unsigned int i = 0; i < vec.size(); i++) {
			PyList_SetItem(val, i, PyUnicode_FromString(vec[i].c_str()));
		}
		PyDict_SetItem(out, key, val);
	}
	$result = out;
}

%{
#define SWIG_FILE_WITH_INIT
#include "../CfgClient.hpp"
%}

%include "std_string.i";

%include "../CfgClient.hpp"

%extend CfgClientException {
	std::string __repr__() {
		return $self->what();
	}
	std::string __str__() {
		return $self->what();
	}
}

%extend CfgClientFatalException {
	std::string __repr__() {
		return $self->what();
	}
	std::string __str__() {
		return $self->what();
	}
}

%extend CfgClient {
%pythoncode {
	def get(self, path, database=AUTO):
	    return self.node_get(database, path)

	def tree_get_dict(self, path, database=AUTO, encoding="json"):
	    import json
	    res = self.tree_get_encoding(database, path, encoding)
	    return json.loads(res)

	def tree_get_full_dict(self, path, database=AUTO, encoding="json"):
	    import json
	    res = self.tree_get_full_encoding(database, path, encoding)
	    return json.loads(res)

	def call_rpc_dict(self, ns, name, input):
	    import json
	    inputStr = json.dumps(input)
	    output = self.call_rpc(ns, name, inputStr)
	    return json.loads(output)
}
}
