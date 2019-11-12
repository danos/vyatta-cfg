#!/usr/bin/perl

# Author: An-Cheng Huang <ancheng@vyatta.com>
# Date: 2007
# Description: script to validate types
# **** License ****
# Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
#
# Copyright (c) 2014-2015 by Brocade Communications Systems, Inc.
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
use lib "/opt/vyatta/share/perl5/";
use Vyatta::TypeChecker;

# validate a value of a specific type
if ($#ARGV < 1) {
  print "usage: vyatta-validate-type.pl [-q] <type> <value>\n";
  exit 1;
}

my $quiet = undef;
if ($ARGV[0] eq '-q') {
  shift;
  $quiet = 1;
}

exit 0 if (Vyatta::TypeChecker::validateType($ARGV[0], $ARGV[1], $quiet));
exit 1;
