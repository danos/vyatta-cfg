#!/usr/bin/perl

# Author: Vyatta <eng@vyatta.com>
# Date: 2007
# Description: Perl script for loading config file at run time.

# **** License ****
# Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
#
# Copyright (c) 2014-2016 by Brocade Communications Systems, Inc.
# All rights reserved.
#
# This code was originally developed by Vyatta, Inc.
# Portions created by Vyatta are Copyright (C) 2007-2013 Vyatta, Inc.
# All Rights Reserved.
#
# SPDX-License-Identifier: GPL-2.0-only
#
# **** End License ****

# $0: config file.

use strict;
use warnings;
use lib "/opt/vyatta/share/perl5/";
use POSIX;
use IO::Prompt;
use Getopt::Long;
use Sys::Syslog qw(:standard :macros);
use Vyatta::Config qw(get_short_config_path);

$SIG{'INT'} = 'IGNORE';

my $etcdir       = $ENV{vyatta_sysconfdir};
my $sbindir      = $ENV{vyatta_sbindir};
my $bootpath     = $etcdir . "/config";
my $load_file    = $bootpath . "/config.boot";
my $url_tmp_file = $bootpath . "/config.boot.$$";
my $ri_name;

sub usage {
    print "Usage: $0 <filename> | $0 routing-instance <ri-name> <filename>\n";
    exit 0;
}

my $mode = 'local';
my $proto;

if ( $#ARGV == 2 && $ARGV[0] eq 'routing-instance' ) {
    $ri_name = $ARGV[1];
    $load_file = $ARGV[2];
} else {
    if ( defined( $ARGV[0] ) ) {
        $load_file = $ARGV[0];
    }
}

my $orig_load_file = $load_file;

if ( $load_file =~ /^[^\/]\w+:\// ) {
    if ( $load_file =~ /^(\w+):\/\/\w/ ) {
        $mode  = 'url';
        $proto = lc($1);
        unless( $proto eq 'tftp' ||
		$proto eq 'ftp'  ||
		$proto eq 'http' ||
		$proto eq 'scp' ) {
	    die "Invalid url protocol [$proto]\n";
        }
    } else {
        print "Invalid url [$load_file]\n";
        exit 1;
    }
}

if ( $mode eq 'local' and !( $load_file =~ /^\// ) ) {
    # relative path
    $load_file = "$bootpath/$load_file";
}

if ( $mode eq 'url' ) {
    if ( !-f '/usr/bin/curl' ) {
        print "Package [curl] not installed\n";
        exit 1;
    }
    my $rc;
    if ($ri_name) {
        $rc = system("$sbindir/vyatta-transfer-url",
                     "--ri=$ri_name",
                     "--outfile=$url_tmp_file",
                     "$load_file");
    } else {
        $rc = system("$sbindir/vyatta-transfer-url",
                     "--outfile=$url_tmp_file",
                     "$load_file");
    }
    if ($rc) {
        print "Can not open remote configuration file $load_file\n";
        exit 1;
    }

    $load_file = $url_tmp_file;
}

# log it
openlog( $0, "", LOG_USER );
my $login = getlogin() || getpwuid($<) || "unknown";
syslog( "warning", "Load config [$orig_load_file] by $login" );

# do config migration via configd since this script is called by unprivileged users directly.
system("cli-shell-api migrateFile $load_file");

# when presenting to users, show shortened /config path
my $shortened_load_file = get_short_config_path($load_file);
print "Loading configuration from '$shortened_load_file'...\n";

my $cobj = new Vyatta::Config;
# "load" => use backend through API
system("cli-shell-api loadFileReportWarnings $load_file");

if (!($cobj->sessionChanged())) {
  print "No configuration changes to commit\n";
  exit 0;
}

print ("\nLoad complete.  Use 'commit' to make changes active.\n");
exit 0;
