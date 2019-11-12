#!/usr/bin/perl

# Author: Arthur Xiong
# Date: 06/21/2010
# Description: Script to generate Vyatta command help strings from 
#              template files

# **** License ****
# Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
#
# Copyright (c) 2014 by Brocade Communications Systems, Inc.
# All rights reserved.
#
# This code was originally developed by Vyatta, Inc.
# Portions created by Vyatta are Copyright (C) 2010-2013 Vyatta, Inc.
# All Rights Reserved.
#
# SPDX-License-Identifier: GPL-2.0-only
#
# **** End License ****

use strict;
use warnings;
use File::Find;

my @command_line;
my $cfgdir;
my $feature_name="";

#
sub get_parameter {
    open my $nd, '<', $_  or die "$_ can't be opened";
    my $help_string = "";

    while (<$nd>) {
        next unless /^help:\s*(\S.*)$/;
        $help_string .= "\|$1" if /^help:\s*(\S.*)$/;
        last;
    }
    close $nd;
    return $help_string;
}

#
# Generate command with parameter
# 
sub wanted {
    return unless ( $_ eq 'node.def' );
    my $parameter = get_parameter($File::Find::name);

    my $dir = $File::Find::dir;
    return if $dir =~ /^.*\/\.\S(|-)\S*\/?.*$/;
    if ( $feature_name ne "" ) {
        return unless $dir =~ /^.*\/$feature_name(|\/.*)$/;
    }

    $dir =~ s/^.*\/(|\S*\-)(templates)(|\-(cfg|op))\///;
#    $dir =~ s/\// /g;
    $dir .= "  " . $parameter . "\n";
    
    push @command_line, $dir;
    return 1;
}

# main program

die "Usage: helpstring.pl <path-to-template> [<feature-name>]"
 unless $#ARGV == 0 or $#ARGV == 1;
$cfgdir = $ARGV[0];
$feature_name = $ARGV[1] if $#ARGV == 1;
die "$cfgdir does not exist!" unless -d $cfgdir;

# walk template file tree
find( \&wanted, $cfgdir );

print @command_line; 
