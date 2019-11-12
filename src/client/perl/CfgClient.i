/*
 * Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
 *
 * Copyright (c) 2015 by Brocade Communications Systems, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 */
%module "Vyatta::Configd"

//Follow PBP on naming. Leave module names CamelCased and use
//under_case for function names. Since this is the first release
//of this new perl API we can use this style.
%rename("%(undercase)s", notregexmatch$name="CfgClient*") "";
%rename("%s", regexmatch$name="^[A-Z_]+$") ""; //don't rename constants
%rename("Client") CfgClient;

//Perl doesn't need these classes, exceptions are handled in the
//C++ wrapper code.
%ignore CfgClientFatalException;
%ignore CfgClientException;

//string2vec is internal to the C++ code
%ignore string2vec;

%typemap(throws) CfgClientFatalException %{
	std::string str = $1.what();
	std::replace(str.begin(), str.end(), '\n', ' ');
	croak("%s", str.c_str());
%}

%typemap(throws) CfgClientException %{
	//Perl best practices says that errors
	//should always throw exceptions in perl
	//perl exception handling is weird, but 
	//this really is best practice.
	std::string str = $1.what();
	std::replace(str.begin(), str.end(), '\n', ' ');
	croak("%s", str.c_str());
%}

//Map returned std::map<std::string, std::string> to perl hashes
%typemap(out) std::map<std::string, std::string> {
	std::map<std::string, std::string> &m = $1;
	HV *href = newHV();
	std::map<std::string, std::string>::iterator it = m.begin();
	for (; it != m.end(); ++it) {
		const char *key = (*it).first.c_str();
		const char *val = (*it).second.c_str();
		hv_store(href, key, strlen(key), newSVpv(val, 0), 0);
	}
	$result = sv_2mortal(newRV((SV *) href));
	argvi++; //this is interal to what swig does, why it makes us do it is anyone's guess
}

//Map returned std::map<std::string, std::vector<std::string>> to perl hashes
%typemap(out) std::map<std::string, std::vector<std::string> > {
	std::map<std::string, std::vector<std::string> > &m = $1;
	HV *href = newHV();
	std::map<std::string, std::vector<std::string> >::iterator it = m.begin();
	for (; it != m.end(); ++it) {
		const char *key = (*it).first.c_str();
		std::vector<std::string> &vec = (*it).second;
		AV *val = newAV();
		for (unsigned int i = 0; i < vec.size(); i++) {
			av_push(val, newSVpv(vec[i].c_str(), 0));
		}
		hv_store(href, key, strlen(key), newRV((SV *)val), 0);
	}
	$result = sv_2mortal(newRV((SV *) href));
	argvi++; //this is interal to what swig does, why it makes us do it is anyone's guess
}

//Map input strings, or arrayrefs to c++ vectors
%typemap(in) std::vector<std::string> & (std::vector<std::string> vec) {
	int i = 0;
	I32 num = 0;
	if (!SvROK($input)) {
		std::string str = SvPV_nolen($input);
		string2vec(str, vec);
		$1 = &vec;
	} else if (SvTYPE(SvRV($input)) != SVt_PVAV) {
		XSRETURN_UNDEF;
    } else {
		SV** tv;
		AV* av = (AV *) SvRV($input);
		num = av_len(av);
		/* if input array is empty, vector will be empty as well. */
		for (i = 0; i <= num; i++) {
			tv = av_fetch(av, i, 0);
			std::string str = SvPV_nolen(*tv);
			vec.push_back(str);
		}
		$1 = &vec;
	}
}

//Map output vectors to multiple return values.
%typemap(out) std::vector<std::string> {
	//return perl ARRAYS using stack returns
	//this avoids needing an additional layer to dereference
	//and return.
	std::vector<std::string> &vec = $1;
	I32 gimme = GIMME_V;
	switch (gimme) {
	case G_VOID:
		{
			//don't bother no one wants the values
			XSRETURN(0);
		}
		break;
	case G_SCALAR:
		{
			//return a reference to the array
			AV *results = newAV();
			for (unsigned int i = 0; i < vec.size(); i++) {
				av_push(results, newSVpv(vec[i].c_str(), 0));
			}
			$result = sv_2mortal(newRV((SV *) results));
			argvi++;
		}
		break;
	case G_ARRAY:
		{
			for (unsigned int i = 0; i < vec.size(); i++) {
				if (argvi >= items) {
					EXTEND(sp, 1);
				}
				$result = sv_2mortal(newSVpv(vec[i].c_str(), 0));
				argvi++;
			}
		}
		break;
	}
}

%{
#define SWIG_FILE_WITH_INIT
#include "../CfgClient.hpp"
#undef seed
#include <algorithm>
%}

%include "std_string.i";

%include "../CfgClient.hpp"

//Provide some convenience functions where perl can do things better
//than C++
%perlcode %{
	*AUTO = *Vyatta::Configd::Client::AUTO;
	*RUNNING = *Vyatta::Configd::Client::RUNNING;
	*CANDIDATE = *Vyatta::Configd::Client::CANDIDATE;
	*EFFECTIVE = *Vyatta::Configd::Client::EFFECTIVE;
	*UNCHANGED = *Vyatta::Configd::Client::UNCHANGED;
	*CHANGED = *Vyatta::Configd::Client::CHANGED;
	*ADDED = *Vyatta::Configd::Client::ADDED;
	*DELETED = *Vyatta::Configd::Client::DELETED;
	*LEAF = *Vyatta::Configd::Client::LEAF;
	*MULTI = *Vyatta::Configd::Client::MULTI;
	*CONTAINER = *Vyatta::Configd::Client::CONTAINER;
	*TAG = *Vyatta::Configd::Client::TAG;

	our @EXPORT_OK = qw(
		$AUTO
		$CANDIDATE
		$RUNNING
		$EFFECTIVE
		$CHANGED
		$UNCHANGED
		$ADDED
		$DELETED
		$LEAF
		$MULTI
		$TAG
		$CONTAINER
	);
	package Vyatta::Configd::Client;
	use JSON;
	sub get {
		my ($self, $path, $database) = @_;
		$database = $AUTO unless defined $database;
		$self->node_get($database, $path);
	}
	sub tree_get_hash {
		my ($self, $path, $opts) = @_;
		$opts->{'encoding'} = "json"
			unless defined $opts->{'encoding'};
		$opts->{'database'} = $AUTO
			unless defined $opts->{'database'};
		my $res = $self->tree_get_encoding($opts->{'database'}, $path, $opts->{'encoding'});
		return unless defined $res;
		return from_json($res);
	}
	sub tree_get_full_hash {
		my ($self, $path, $opts) = @_;
		$opts->{'encoding'} = "json"
			unless defined $opts->{'encoding'};
		$opts->{'database'} = $AUTO
			unless defined $opts->{'database'};
		my $res = $self->tree_get_full_encoding($opts->{'database'}, $path, $opts->{'encoding'});
		return unless defined $res;
		return from_json($res);
	}
	sub call_rpc_hash {
		my ($self, $ns, $name, $input) = @_;
		my $inputStr = to_json($input);
		return unless defined $inputStr;
		my $output = $self->call_rpc($ns, $name, $inputStr);
		return unless defined $output;
		return from_json($output);
	}
%}
