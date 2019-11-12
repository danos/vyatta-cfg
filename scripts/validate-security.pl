#!/usr/bin/perl
#
# Copyright (c) 2018-2019, AT&T Intellectual Property. All rights reserved.
#
# SPDX-License-Identifier: GPL-2.0-only
#

use strict;
use warnings;

use lib "/opt/vyatta/share/perl5/";
use Vyatta::Platform qw(check_security_features);

my $ret = Vyatta::Platform::check_security_features();

exit $ret;
