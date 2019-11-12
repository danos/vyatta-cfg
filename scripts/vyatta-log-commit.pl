#!/usr/bin/perl
#
# **** License ****
# Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
#
# Copyright (c) 2014 by Brocade Communications Systems, Inc.
# All rights reserved.
#
# This code was originally developed by Vyatta, Inc.
# Portions created by Vyatta are Copyright (C) 2012-2013 Vyatta, Inc.
# All Rights Reserved.
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Author: Deepti Kulkarni	 
# Date: Feb 2012
# Description: Script to log active configuration commits to syslog.
#
# **** End License ****

use strict;
use warnings;

use Sys::Syslog qw(:standard :macros);
use POSIX qw(ttyname);

#
# main
#
my $status = $ENV{'COMMIT_STATUS'};
my $commit_status;

if (defined($status) && $status eq 'SUCCESS') {
    my $cur_user = getlogin() || getpwuid($<) || "unknown";

    openlog("commit", "", LOG_USER);
    syslog (LOG_NOTICE, "Successful change to active configuration by user $cur_user");
    closelog();
}
#end of script
