#!/usr/bin/perl
# Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
#
# Copyright (c) 2014 by Brocade Communications Systems, Inc.
# All rights reserved.
#
# SPDX-License-Identifier: GPL-2.0-only
use strict;
use warnings;

use lib "/opt/vyatta/share/perl5/";
use Vyatta::Config;

sub get_deleted_addrs {
    my @dev_addrs;
    my $config = new Vyatta::Config("interfaces");
    foreach my $type ($config->listNodes(), $config->listDeleted()) {
        foreach my $if ($config->listNodes("$type"), $config->listDeleted("$type")) {
            foreach my $addr ($config->returnDeletedValues("$type $if address")) {
                # Never delete system loopback addresses
                next if ($if eq "lo" && ($addr =~ /^127.0.0.1\// ||  $addr =~ /^::1\//));
                push(@dev_addrs, "$if $addr");
            }

            foreach my $vif ($config->listNodes("$type $if vif"), $config->listDeleted("$type $if vif")) {
                foreach my $addr ($config->returnDeletedValues("$type $if vif $vif address")) {
                    push(@dev_addrs, "${if}.$vif $addr");
                }
            }
        }
    }
    return @dev_addrs;
}

foreach my $dev_addr (get_deleted_addrs()) {
    my ($intf, $addr) = split(' ', $dev_addr);
    system("/opt/vyatta/sbin/vyatta-address delete $intf $addr");
}
