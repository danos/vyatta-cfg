#!/usr/bin/perl

# Author: Vyatta <eng@vyatta.com>
# Date: 2007
# Description: script to save the configuration

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

use strict;
use warnings;
use lib "/opt/vyatta/share/perl5";
use File::Sync qw(fsync);
use FileHandle;
use IO::Prompt;
use Getopt::Long;
use File::Basename;
use File::Temp;
use Vyatta::Config qw(get_short_config_path);

umask 0077;

my $etcdir = "/opt/vyatta/etc";
my $sbindir = "/opt/vyatta/sbin";
my $bootpath = $etcdir . "/config";
my $save_file = $bootpath . "/config.boot";
my $url_tmp_file = $bootpath . "/config.boot.$$";
my $ri_name;

my $show_default = 1;
my $no_defaults;

GetOptions(
  'no-defaults' => \$no_defaults,
);

if ($no_defaults) {
  $show_default = 0;
}

if ($#ARGV == 0) {
  $save_file = $ARGV[0];
}
elsif ($#ARGV == 2 && $ARGV[0] eq 'routing-instance') {
  $ri_name = $ARGV[1];
  $save_file = $ARGV[2];
}
elsif ($#ARGV != -1) {
  print "Usage: save [routing-instance <ri_name>] [config_file_name] [--no-defaults]\n";
  exit 1;
}


my $mode = 'local';
my $proto;

if ($save_file =~ /^[^\/]\w+:\//) {
  if ($save_file =~ /^(\w+):\/\/\w/) {
    $mode = 'url';
    $proto = lc($1);
    if ($proto eq 'tftp') {
    } elsif ($proto eq 'ftp') {
    } elsif ($proto eq 'http') {
    } elsif ($proto eq 'scp') {
    } else {
      print "Invalid url protocol [$proto]\n";
      exit 1;
    }
  } else {
    print "Invalid url [$save_file]\n";
    exit 1;
  }
}

if ($mode eq 'local' and !($save_file =~ /^\//)) {
  # relative path
  $save_file = "$bootpath/$save_file";
}

my $version_str = `$sbindir/vyatta_current_conf_ver.pl`;
# when presenting to users, show shortened /config path
my $shortened_save_file = get_short_config_path($save_file);
print "Saving configuration to '$shortened_save_file'...\n";

my $save;
my $tmp_save_fname;
my ( $filename, $basedir, ) = fileparse($save_file);
if ($mode eq 'local') {
    # first check if this file exists, and if so ensure this is a config file.
    if (-e $save_file) {
	my $result = `grep ' === vyatta-config-version:' $save_file`;
	if (!defined $result || length($result) == 0) {
	    print "File exists and is not a Vyatta configuration file, aborting save!\n";
	    exit 1;
	}
    }

    $save = File::Temp->new(
        TEMPLATE => 'tmp_configXXXXXXXX',
        DIR      => $basedir,
        UNLINK   => 1
    ) or die "Failed to create a tmp_config file:$!\n";

    $tmp_save_fname = $save->filename;
} elsif ($mode eq 'url') {
    die "Package [curl] not installed\n" unless ( -f '/usr/bin/curl');

    open $save, '>', $url_tmp_file
	or die "Cannot open file '$url_tmp_file': $!\n";
}

select $save;
my $show_cmd = 'cli-shell-api showConfig --show-active-only --show-ignore-edit';
if ($show_default) {
  $show_cmd .= ' --show-show-defaults';
}
open(my $show_fd, '-|', $show_cmd) or die 'Cannot execute config output';
while (<$show_fd>) {
  print or die "Failed to write in config file $save: $!\n";
}
close($show_fd);
print $version_str;
$| = 1;
select STDOUT;

fsync $save;
close $save;

if ( ( $mode eq 'local' ) and ( -s $tmp_save_fname ) ) {
  rename $tmp_save_fname, $save_file;
  open my $fh_basedir, '<', $basedir
    or die "Failed to open filehandle $basedir: $!";
  fsync($fh_basedir) or die "Failed to fsync $basedir: $!\n";
  close $fh_basedir;
}

if ($mode eq 'url') {
  my $rc;
  if ($ri_name) {
    $rc = system("$sbindir/vyatta-transfer-url",
                 "--ri=$ri_name",
                 "--infile=$url_tmp_file",
                 "$save_file");
  } else {
    $rc = system("$sbindir/vyatta-transfer-url",
                 "--infile=$url_tmp_file",
                 "$save_file");
  }
  system("rm -f $url_tmp_file");
  if ($rc) {
    print "Error saving $save_file\n";
    exit 1;
  }
}

print "Done\n";
exit 0;
