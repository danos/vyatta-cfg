# Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
#
# Copyright (C) 2010-2013 Vyatta, Inc.
#
# Copyright (C) 2010, 2014, 2017 by Brocade Communications Systems, Inc.
# All rights reserved.
#
# SPDX-License-Identifier: LGPL-2.1-only

package Cstore;

use 5.010000;
use strict;
use warnings;

require Exporter;
use AutoLoader qw(AUTOLOAD);

our @ISA = qw(Exporter);

# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.

# This allows declaration	use Cstore ':all';
# If you do not need this, moving things directly into @EXPORT or @EXPORT_OK
# will save memory.
our %EXPORT_TAGS = ( 'all' => [ qw(
	
) ] );

our @EXPORT_OK = ( @{ $EXPORT_TAGS{'all'} } );

our @EXPORT = qw(
	
);

our $VERSION = '0.01';

require XSLoader;
XSLoader::load('Cstore', $VERSION);

# Preloaded methods go here.

# Autoload methods go after =cut, and are processed by the autosplit program.

1;
__END__
=head1 NAME

Cstore - Perl binding for the Vyatta Cstore library

=head1 SYNOPSIS

  use Cstore;
  my $cstore = new Cstore;

=head1 DESCRIPTION

This module provides the Perl binding for the Vyatta Cstore library.

=head2 EXPORT

None by default.

=head1 SEE ALSO

For more information on the Cstore library, see the documentation and
source code for the main library.

=head1 AUTHOR

Vyatta Package Maintainers E<lt>support@brocade.comE<gt>

=head1 COPYRIGHT

Copyright (C) 2010, 2014, 2017 by Brocade Communications Systems, Inc.
All rights reserved.

=cut
