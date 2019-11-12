/*
 * Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
 *
 * Copyright (c) 2014 by Brocade Communications Systems, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 */

#ifndef CPATH_HPP_
#define CPATH_HPP_
#include <string>
#include <vector>
#include <uriparser/Uri.h>

/*
 *  URI_MULT is defined by the uriparser documenation to need to be 6 when expanding breaks
 *  http://uriparser.sourceforge.net/doc/html/Uri_8h.html#a0d501e7c2a6abe76b729ea72e123dea8
 */
#define URI_MULT 6

class Cpath
{
public:
	Cpath()  : _data() {};
	Cpath(const Cpath& p) : _data() {
		operator=(p);
	};
	Cpath(const std::string &comps) : _data() {
		size_t pos = 0;
		std::string token;
		std::string s = comps;
		size_t startpos = s.find_first_not_of(" \t/");
		if(startpos != std::string::npos)
		{
			s = s.substr(startpos);
		}
		while ((pos = s.find('/')) != std::string::npos) {
			token = s.substr(0, pos);
			push(url_unescape(token));
			s.erase(0, pos + 1);
		}
		token = s.substr(0, pos);
		push(url_unescape(token));
	}
	Cpath(const char *comps[], size_t num_comps) {
		for (size_t i = 0; i < num_comps; i++) {
			push(comps[i]);
		}
	};
	~Cpath() {};

	void push(const char *comp) {
		_data.push_back(comp);
	};
	void push(const std::string &comp) {
		_data.push_back(comp.c_str());
	};
	void push_unescape(const char *comp) {
		_data.push_back(url_unescape(comp));
	};
	void pop() {
		_data.pop_back();
	};
	void pop(std::string &last) {
		last = _data.back();
		_data.pop_back();
	};
	void clear() {
		_data.clear();
	};

	Cpath& operator=(const Cpath& p) {
		_data = p._data;
		return *this;
	};

	Cpath& operator+=(const std::string &s) {
		push(s);
		return *this;
	};

	Cpath& operator+=(const char *s) {
		push(s);
		return *this;
	};

	bool operator==(const Cpath& rhs) const {
		return (_data == rhs._data);
	};

	const char *operator[](size_t idx) const {
		return _data[idx].c_str();
	};

	size_t size() const {
		return _data.size();
	};

	const char *back() const {
		return (size() > 0 ? _data.back().c_str() : NULL);
	};

	std::string to_string() const {
		std::string out;
		for (size_t i = 0; i < size(); i++) {
			if (i > 0) {
				out += " ";
			}
			out += url_escape((*this)[i]);
		}
		return out;
	};

	std::string to_path_string() const {
		std::string out = "/";
		for (size_t i = 0; i < size(); i++) {
			if (i > 0) {
				out += "/";
			}
			out += url_escape((*this)[i]);
		}
		return out;
	};

	bool at_root() {
		return _data.size() == 0;
	};

private:
	struct CpathParams {
		static const char separator = 0;
		static const size_t static_num_elems = 24;
		static const size_t static_buf_len = 256;
	};

	std::vector<std::string> _data;
	std::string url_escape(const char* s) const {
		char *out = (char *)malloc(strlen(s) * URI_MULT);
		uriEscapeA(s, out, URI_FALSE, URI_TRUE);
		std::string outs = std::string(out);
		free(out);
		return outs;
	}
	std::string url_unescape(std::string s) const {
		char *out = strdup(s.c_str());
		uriUnescapeInPlaceA(out);
		std::string outs = std::string(out);
		free(out);
		return outs;
	}
};

#endif
