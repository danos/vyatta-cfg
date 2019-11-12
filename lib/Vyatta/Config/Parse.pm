# **** License ****
# Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
#
# Copyright (c) 2014-2015 by Brocade Communications Systems, Inc.
# All rights reserved.
#
# SPDX-License-Identifier: LGPL-2.1-only
#
# **** End License ****

package  Vyatta::Config::Parse;

use strict;

use lib '/opt/vyatta/share/perl5';

use Vyatta::Config;
use JSON;

sub new {
    my ($that, $level) = @_;
    my $class = ref($that) || $that;
    my $cfg = new Vyatta::Config($level);
    my $tree = $cfg->getTree();
    return unless defined $tree;
    my $self = from_json($tree);
    bless $self, $class;
    return $self;
}
1;
