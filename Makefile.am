SUBDIRS = . perl_dmod cpputest
completiondir		= /etc/bash_completion.d
logrotatedir		= /etc/logrotate.d
defaultdir		= /etc/default
share_perl5dir		= /opt/vyatta/share/perl5/Vyatta
share_perl5configdir		= /opt/vyatta/share/perl5/Vyatta/Config
testdir                 = /opt/vyatta/share/test
bindir			= /opt/vyatta/bin
yangdir			= /usr/share/configd/yang
mandir			= /usr/share/man/

AM_CFLAGS = -std=gnu99 -pedantic -O2 -g -I src -Wall -Werror
AM_CXXFLAGS = -std=c++0x -O2 -g -I src -Wall -Werror -Wno-deprecated
AM_YFLAGS = -d --name-prefix=yy_`basename $* .y`_
AM_LFLAGS = --prefix=yy_`basename $* .l`_ -olex.yy.c

completion_DATA		= etc/bash_completion.d/vyatta-cfg
logrotate_DATA		= etc/logrotate.d/vyatta-config-logs

functiondir	= $(datadir)/vyatta-cfg/functions
function_DATA	= functions/script-wrapper
interpdir	= $(functiondir)/interpreter/
interp_DATA	= functions/interpreter/vyatta-cfg-run

yang_DATA = yang/vyatta-security-v1.yang
yang_DATA += yang/vyatta-system-v1.yang
yang_DATA += yang/vyatta-policy-v1.yang
yang_DATA += yang/vyatta-resources-v1.yang
yang_DATA += yang/vyatta-services-v1.yang
yang_DATA += yang/vyatta-types-v1.yang

src/cparse/cparse.cpp: src/cparse/cparse.ypp src/cparse/cparse_def.h
	bison -p cparse_ --defines=src/cparse/cparse.h -o $@ $<

src/cparse/cparse_lex.c: src/cparse/cparse_lex.l src/cparse/cparse.cpp
	flex -P cparse_ -o $@ $<

src/cli_def.c: src/cli_def.l
	flex --prefix=yy_cli_def_ -o $@ $<

src/cli_val.c: src/cli_val.l
	flex --prefix=yy_cli_val_ -o $@ $<

bin_SCRIPTS = scripts/vcli
bin_SCRIPTS += scripts/vyatta-rpc-reboot
bin_SCRIPTS += scripts/configure
bin_SCRIPTS += scripts/list-configuration-files

CLEANFILES = src/cli_parse.c src/cli_parse.h src/cli_def.c src/cli_val.c
CLEANFILES += src/cparse/cparse.cpp src/cparse/cparse.h
CLEANFILES += src/cparse/cparse_lex.c

LDADD	   = -lstdc++
LDADD	   += -luriparser

vincludedir = $(includedir)/vyatta-cfg
vinclude_HEADERS = src/cli_cstore.h

lib_LTLIBRARIES			= src/libvyatta-config.la
src_libvyatta_config_la_LDFLAGS	= -version-info 2:0:0
src_libvyatta_config_la_SOURCES	= src/client/connect.c
src_libvyatta_config_la_SOURCES += src/client/session.c
src_libvyatta_config_la_SOURCES += src/client/error.c
src_libvyatta_config_la_SOURCES	+= src/client/node.c
src_libvyatta_config_la_SOURCES	+= src/client/template.c
src_libvyatta_config_la_SOURCES	+= src/client/transaction.c
src_libvyatta_config_la_SOURCES	+= src/client/auth.c
src_libvyatta_config_la_SOURCES	+= src/client/log.c
src_libvyatta_config_la_SOURCES	+= src/client/file.c
src_libvyatta_config_la_SOURCES	+= src/client/callrpc.c
src_libvyatta_config_la_SOURCES	+= src/client/completion_env.cpp
src_libvyatta_config_la_SOURCES	+= src/client/ctemplate.cpp
src_libvyatta_config_la_SOURCES	+= src/client/CfgClient.cpp
src_libvyatta_config_la_LIBADD = -lvyatta-util
src_libvyatta_config_la_LIBADD += -lstdc++
src_libvyatta_config_la_LIBADD += -ljansson
src_libvyatta_config_la_LIBADD += -luriparser
src_libvyatta_config_la_CFLAGS = -std=gnu99 -pedantic -g -Wall -Werror -D_GNU_SOURCE
src_libvyatta_config_la_CXXFLAGS = -std=c++0x -g -Wall -Werror -Wno-deprecated

lib_LTLIBRARIES += src/libvyatta-cstore-compat.la
src_libvyatta_cstore_compat_la_LDFLAGS	= -version-info 1:0:0
src_libvyatta_cstore_compat_la_LDFLAGS	+= -Lsrc/.libs
src_libvyatta_cstore_compat_la_LIBADD = -lvyatta-util
src_libvyatta_cstore_compat_la_LIBADD += -lvyatta-config
src_libvyatta_cstore_compat_la_LIBADD += -lperl
src_libvyatta_cstore_compat_la_LIBADD += -luriparser
src_libvyatta_cstore_compat_la_SOURCES = src/compat/cstore-compat.cpp
src_libvyatta_cstore_compat_la_CXXFLAGS = $(AM_CXXFLAGS)
src_libvyatta_cstore_compat_la_CXXFLAGS += -Isrc/client
src_libvyatta_cstore_compat_la_CXXFLAGS += -Lsrc/.libs

vclincdir = $(vincludedir)/client
vclinc_HEADERS = src/client/connect.h
vclinc_HEADERS += src/client/auth.h
vclinc_HEADERS += src/client/node.h
vclinc_HEADERS += src/client/rpc.h
vclinc_HEADERS += src/client/session.h
vclinc_HEADERS += src/client/template.h
vclinc_HEADERS += src/client/file.h
vclinc_HEADERS += src/client/transaction.h
vclinc_HEADERS += src/client/error.h
vclinc_HEADERS += src/client/callrpc.h
vclinc_HEADERS += src/client/mgmt.h
vclinc_HEADERS += src/client/mobj.h
vclinc_HEADERS += src/client/CfgClient.hpp

# SWIG-built language bindings
#
# SWIG gererated code triggers some warnings so we need to turn them
# off. Unfortunately we cannot use noinst_LIBRARIES because generated
# library init routines do not get linked in via a separate archive
# library.

pkgpython3_LTLIBRARIES = src/client/python3/libCfgClient.la
src_client_python3_libCfgClient_la_LDFLAGS = -version-info 1:0:0
src_client_python3_libCfgClient_la_SOURCES = src/client/CfgClient.cpp
src_client_python3_libCfgClient_la_SOURCES += src/client/python3/CfgClient_wrap.cxx
src_client_python3_libCfgClient_la_CXXFLAGS = $(AM_CXXFLAGS) $(python3_CFLAGS)
src_client_python3_libCfgClient_la_CXXFLAGS += -Wno-unused-but-set-variable -Wno-maybe-uninitialized
src_client_python3_libCfgClient_la_LIBADD = src/libvyatta-config.la
src_client_python3_libCfgClient_la_LIBADD += -lvyatta-util
src_client_python3_libCfgClient_la_LIBADD += -lstdc++
src_client_python3_libCfgClient_la_LIBADD += -ljansson
src_client_python3_libCfgClient_la_LIBADD += -luriparser
src_client_python3_libCfgClient_la_LIBADD += $(python3_LIBS)

lib_LTLIBRARIES += src/client/perl/libCfgClientPerl.la
src_client_perl_libCfgClientPerl_la_LDFLAGS = -version-info 1:0:0
src_client_perl_libCfgClientPerl_la_SOURCES = src/client/CfgClient.cpp
src_client_perl_libCfgClientPerl_la_SOURCES += src/client/perl/CfgClient_wrap.cxx
src_client_perl_libCfgClientPerl_la_CXXFLAGS = -std=c++0x -O2 -g -I src -Wall -Werror -Wno-deprecated
src_client_perl_libCfgClientPerl_la_CXXFLAGS += -Wno-unused-but-set-variable -Wno-maybe-uninitialized -Wno-unused-variable
src_client_perl_libCfgClientPerl_la_CXXFLAGS += $(shell perl -MExtUtils::Embed -e ccopts)
src_client_perl_libCfgClientPerl_la_LIBADD = src/libvyatta-config.la
src_client_perl_libCfgClientPerl_la_LIBADD += -lvyatta-util
src_client_perl_libCfgClientPerl_la_LIBADD += -lstdc++
src_client_perl_libCfgClientPerl_la_LIBADD += -ljansson
src_client_perl_libCfgClientPerl_la_LIBADD += -luriparser
src_client_perl_libCfgClientPerl_la_LIBADD += -lperl
src_client_perl_libCfgClientPerl_la_LIBADD += -lpthread

src/client/python3/CfgClient_wrap.cxx src/client/python3/CfgClient.py: src/client/python3/CfgClient.i src/client/CfgClient.hpp
	swig3.0 -Wall -c++ -python -py3 $<

CLEANFILES += src/client/python3/CfgClient_wrap.cxx src/client/python3/CfgClient.py src/client/python3/_CfgClient.so

src/client/perl/CfgClient_wrap.cxx src/client/perl/Configd.pm: src/client/perl/CfgClient.i src/client/CfgClient.hpp
	swig3.0 -Wall -c++ -perl5 $<

CLEANFILES += src/client/perl/CfgClient_wrap.cxx src/client/perl/Configd.pm

sbin_PROGRAMS = src/my_cli_shell_api

src_my_cli_shell_api_SOURCES = src/cli_shell_api.cpp
src_my_cli_shell_api_CPPFLAGS = $(AM_CXXFLAGS)
src_my_cli_shell_api_CPPFLAGS += -Isrc/client
src_my_cli_shell_api_LDFLAGS = -static
src_my_cli_shell_api_LDADD = src/libvyatta-config.la
src_my_cli_shell_api_LDADD += -ljansson

test_PROGRAMS = test/testclient
test_testclient_SOURCES = test/testclient.c
test_testclient_LDFLAGS = -static
test_testclient_LDADD = src/libvyatta-config.la
test_testclient_LDADD += -lvyatta-util
test_testclient_LDADD += -luriparser
test_testclient_LDADD += -ljansson
test_testclient_LDADD += -lstdc++
test_testclient_CPPFLAGS = $(AM_CFLAGS) -D_GNU_SOURCE
test_testclient_CPPFLAGS += -Isrc/client

bin_PROGRAMS = src/cliexec/cliexec
src_cliexec_cliexec_CFLAGS = -std=gnu99 -pedantic -g -Wall -Werror -D_GNU_SOURCE -I src/client
src_cliexec_cliexec_CFLAGS += -fdata-sections -ffunction-sections -Wl,--gc-sections,--print-gc-sections
src_cliexec_cliexec_CXXFLAGS = -std=c++0x -g -Wall -Werror -Wno-deprecated -D_GNU_SOURCE -I src/client
src_cliexec_cliexec_CXXFLAGS += -fdata-sections -ffunction-sections -Wl,--gc-sections,--print-gc-sections
src_cliexec_cliexec_SOURCES = src/cliexec/cliexec.cpp
src_cliexec_cliexec_SOURCES += src/cliexec/cli_parse.c
src_cliexec_cliexec_SOURCES += src/cliexec/cli_new.c
src_cliexec_cliexec_SOURCES += src/cliexec/cli_objects.c
src_cliexec_cliexec_SOURCES += src/cliexec/cstore-c.cpp
src_cliexec_cliexec_SOURCES += src/cliexec/cstore-varref.cpp
src_cliexec_cliexec_SOURCES += src/cliexec/cli_val.c
src_cliexec_cliexec_SOURCES += src/cliexec/cli_def.c
src_cliexec_cliexec_LDFLAGS = -static
src_cliexec_cliexec_LDADD = src/libvyatta-config.la
src_cliexec_cliexec_LDADD += -lstdc++
src_cliexec_cliexec_LDADD += -luriparser
src_cliexec_cliexec_LDADD += -lvyatta-util

src/cliexec/cli_def.c: src/cliexec/cli_def.l
	flex --prefix=yy_cli_def_ -o $@ $<

src/cliexec/cli_val.c: src/cliexec/cli_val.l
	flex --prefix=yy_cli_val_ -o $@ $<

src/cliexec/cli_parse.c src/cliexec/cli_parse.h: src/cliexec/cli_parse.y
	bison -y --name-prefix=yy_cli_parse_ -o src/cliexec/cli_parse.c --defines=src/cliexec/cli_parse.h $<

CLEANFILES += src/cliexec/cli_def.c src/cliexec/cli_val.c
CLEANFILES += src/cliexec/cli_parse.c src/cliexec/cli_parse.h

sbin_SCRIPTS = scripts/vyatta-validate-type.pl
sbin_SCRIPTS += scripts/vyatta-boot-config-loader
sbin_SCRIPTS += scripts/vyatta-config-gen-sets.pl
sbin_SCRIPTS += scripts/vyatta-save-config.pl
sbin_SCRIPTS += scripts/vyatta-load-config.pl
sbin_SCRIPTS += scripts/vyatta-cfg-notify
sbin_SCRIPTS += scripts/vyatta-log-commit.pl
sbin_SCRIPTS += scripts/vyatta-delete-addrs.pl
sbin_SCRIPTS += scripts/vyatta-transfer-url
sbin_SCRIPTS += scripts/allowed-nodes
sbin_SCRIPTS += scripts/validate-security.pl

share_perl5_DATA = lib/Vyatta/Config.pm
share_perl5_DATA += lib/Vyatta/ConfigLoad.pm
share_perl5config_DATA = lib/Vyatta/Config/Parse.pm

default_DATA = etc/default/vyatta-cfg
default_DATA += etc/default/vyatta-load-boot

man_MANS = man/man3/auth.h.3 man/man3/connect.h.3 man/man3/log.h.3 man/man3/mobj.h.3 man/man3/rpc.h.3
man_MANS += man/man3/transaction.h.3 man/man3/error.h.3 man/man3/mgmt.h.3 man/man3/node.h.3 man/man3/session.h.3 man/man3/template.h.3  man/man3/file.h.3

cpiop = find  . ! -regex '\(.*~\|.*\.bak\|.*\.swp\|.*\#.*\#\)' -print0 | \
	cpio -0pd

$(man_MANS):
	doxygen Doxyfile

all-local: $(man_MANS)

install-exec-hook:
	mkdir -p $(DESTDIR)$(sysconfdir)
	mkdir -p $(DESTDIR)$(yangdir)
	cd yang && $(cpiop) $(DESTDIR)$(yangdir)
	chrpath -d src/client/python3/.libs/libCfgClient.so.1.0.0
	cp src/client/python3/.libs/libCfgClient.so.1.0.0 src/client/python3/_configd.so
	mkdir -p $(DESTDIR)/usr/lib/python3/dist-packages/vyatta
	cp src/client/python3/configd.py $(DESTDIR)/usr/lib/python3/dist-packages/vyatta
	cp src/client/python3/_configd.so $(DESTDIR)/usr/lib/python3/dist-packages/vyatta


	mkdir -p $(DESTDIR)/opt/vyatta/share/perl5/auto/Vyatta/Configd
	mkdir -p $(DESTDIR)/opt/vyatta/share/perl5/Vyatta
	chrpath -d src/client/perl/.libs/libCfgClientPerl.so.1.0.0
	cp src/client/perl/.libs/libCfgClientPerl.so.1.0.0 $(DESTDIR)/opt/vyatta/share/perl5/auto/Vyatta/Configd/Configd.so
	cp src/client/perl/Configd.pm $(DESTDIR)/opt/vyatta/share/perl5/Vyatta
