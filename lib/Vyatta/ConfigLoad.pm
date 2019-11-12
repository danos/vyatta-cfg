# Author: Vyatta <eng@vyatta.com>
# Date: 2007
# Description: Perl module for loading configuration.

# **** License ****
# Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
#
# Copyright (c) 2014 by Brocade Communications Systems, Inc.
# All rights reserved.
# 
# This code was originally developed by Vyatta, Inc.
# Portions created by Vyatta are Copyright (C) 2006-2013 Vyatta, Inc.
# All Rights Reserved.
#
# SPDX-License-Identifier: LGPL-2.1-only

package Vyatta::ConfigLoad;
use strict;

our @EXPORT = qw(getStartupConfigStatements loadConfigHierarchy getConfigDiff);
use base qw(Exporter);

use sort 'stable';
use lib "/opt/vyatta/share/perl5";
use XorpConfigParser; 
use Vyatta::Config;

my @all_nodes = ();
my @all_naked_nodes = ();

my @disable_list = ();


sub match_regex {
  my ($pattern, $str) = @_;
  $pattern =~ s/^(.*)$/\^$1\$/;
  return ($str =~ m/$pattern/) ? 1 : 0;
}


sub applySingleQuote {
  my @return = ();
  foreach (@_) {
    # change all single quotes to "'\''" since we're going to single-quote
    # every component of the command
    if (/^'(.*)'$/) {
      $_ = $1;
    }
    $_ =~ s/'/'\\''/g;
    # single-quote every component of the command
    if (/^'.*'$/) {
      push @return, $_; 
    } elsif (/^"(.*)"$/) {
      push @return, "'$1'";
    } else {
      push @return, "'$_'"; 
    }
  }
  return @return;
}

my @comment_list = ();

sub enumerate_branch {
  my $cur_node = shift;
  my @cur_path = @_;
  # name not defined at root level
  if (defined($cur_node->{'name'})) {
    my $name = $cur_node->{'name'};
    if ($name =~ /^\s*(\S+)\s+(\S.*)$/) {
      push @cur_path, ($1, $2);
    } else {
      push @cur_path, $name;
    }
  }
  my $terminal = 0;
  if (!defined($cur_node->{'children'})) {
      $terminal = 1;
  } else {
      my $comment;
      foreach (@{$cur_node->{'children'}}) {
	  if (defined($_->{'comment'})) {
	      $comment = $_->{'comment'};
	  }

	  if (defined($_->{'name'})) {
	      if (defined $comment) {
		  push @comment_list, join(" ", (@cur_path, $_->{'name'}, "\"" . $comment . "\""));
		  $comment = undef;
	      }
	      else {
		  #need to check for existance of .comment file here.
		  push @comment_list, join(" ", (@cur_path, $_->{'name'}, "\"\""));
	      }
	      enumerate_branch($_, @cur_path);
	      $terminal = 0;
	  }

      }
  }

  if (defined($cur_node->{'disable'})) {
      push @disable_list, join(" ",@cur_path);
  }

  if ($terminal) {
    my $val = $cur_node->{'value'};
    if (defined($val)) {
      push @cur_path, $val;
    }
    push @all_naked_nodes, [ @cur_path ];
    my @qpath = applySingleQuote(@cur_path);
    push @all_nodes, [\@qpath, 0];
  }
}

# $0: config file to load
# return: list of all config statement
sub getStartupConfigStatements {
  # clean up the lists first
  @all_nodes = ();
  @all_naked_nodes = ();

  my $load_cfg = shift;
  if (!defined($load_cfg)) {
    return ();
  }

  my $comments = shift;
  
  my $xcp = new XorpConfigParser();
  $xcp->parse($load_cfg);
  my $root = $xcp->get_node( () );
  if (!defined($root)) {
    return ();
  }
  enumerate_branch($root, ( ));

  if (defined $comments && $comments eq 'true') {
      #add comment commands to all nodes
      foreach my $c (@comment_list) {
	  if ($c !~ /\"\"$/) {
	      my @pth = split(" ",'comment ' . $c);
	      push @all_nodes, [\@pth, 1];
	  }
      }
  }
  my %conf = (
              'set' => \@all_nodes,
              'deactivate' => \@disable_list,
             );
  return %conf;
}

my %node_order = ();

# $0: ref of list of parsed naked statements.
# return: hash containing the config hierarchy.
sub generateHierarchy {
  my @node_list = @{$_[0]};
  my %hash = ();
  %node_order = ();
  my $order = 0;
  foreach my $node (@node_list) {
    my @path = @{$node};
    my $path_str = join ' ', @path;
    $node_order{$path_str} = $order;
    $order++;
    my $cur_ref = \%hash;
    foreach (@path) {
      if (!defined($cur_ref->{$_})) {
        $cur_ref->{$_} = { };
      }
      $cur_ref = $cur_ref->{$_};
    }
  }
  return %hash;
}

# $0: config file to load.
# return: hash containing the config hierarchy.
sub loadConfigHierarchy {
  # clean up the lists first
  @all_nodes = ();
  @all_naked_nodes = ();

  my $load_cfg = shift;
  if (!defined($load_cfg)) {
    return ();
  }

  #allows loading from arbritary root
  my $root_node = shift;
  
  my $xcp = new XorpConfigParser();
  $xcp->parse($load_cfg);
  my $root;
  my @root_path;
  if (defined($root_node)) {
      my $tmp = [split("/",$root_node)];
      $root = $xcp->get_node($tmp);
      my @tmp2 = split("/",$root_node);
      @root_path = pop(@tmp2);
      @root_path = @tmp2;
  }
  else {
      $root = $xcp->get_node( () );
  }
  if (!defined($root)) {
    return ();
  }
  enumerate_branch($root, @root_path);

  return generateHierarchy(\@all_naked_nodes);
}

# $0: ref of hierarchy root.
# $1: display prefix.
sub printHierarchy {
  my $cur_ref = shift;
  my $prefix = shift;
  foreach (sort keys %{$cur_ref}) {
    print "$prefix$_";
    if (scalar(keys %{$cur_ref->{$_}}) == 0) {
      print " (terminal)\n";
      next;
    } else {
      print "\n";
    }
    printHierarchy($cur_ref->{$_}, "$prefix    ");
  }
}

# $0: hash ref representing a "multi:" node.
# $1: array ref representing current config path.
# returns the list of node values sorted by the original order.
sub getSortedMultiValues {
  my $nref = $_[0];
  my @npath = @{$_[1]};
  my $path_str = join ' ', @npath;
  my @list = ();
  foreach (keys %{$nref}) {
    my $key = "$path_str $_";
    push @list, [ $_, $node_order{$key} ];
  }
  my @slist = sort { ${$a}[1] <=> ${$b}[1] } @list;
  @slist = map { ${$_}[0] } @slist; 
  return @slist;
}

my $active_cfg = undef;
my $new_cfg_ref = undef;
my @delete_list = ();

# find specified node's values in active config that have been deleted from
# new config.
# $0: hash ref at the current hierarchy level (new config)
# $1: array ref representing current config path (active config)
sub findDeletedValues {
  my $new_ref = $_[0];
  my @active_path = @{$_[1]};

  $active_cfg->setLevel(join ' ', @active_path);
  my ($is_multi, $is_text) = $active_cfg->parseTmpl();
  if ($is_multi) {
    # for "multi:" nodes, need to sort the values by the original order.
    my @nvals = getSortedMultiValues($new_ref, \@active_path);
    if ($is_text) {
      @nvals = map { /^"(.*)"$/ ? $1 : $_ }@nvals;
    }
    my @ovals = $active_cfg->returnOrigValues('');
    my %comp_hash = $active_cfg->compareValueLists(\@ovals, \@nvals);
    foreach (@{$comp_hash{'deleted'}}) {
      my @plist = applySingleQuote(@active_path, $_);
      push @delete_list, [\@plist, 0];
    }
  } else {
    # do nothing. if a single-value leaf node is deleted, it should have
    # been detected at the previous level. since we are already at node.val,
    # it can only be "added" or "changed", handled later.
  }
}

# find nodes in active config that have been deleted from new config.
# $0: hash ref at the current hierarchy level (new config)
# $1: array ref representing current config path (active config)
sub findDeletedNodes {
  my $new_ref = $_[0];
  my @active_path = @{$_[1]};
  $active_cfg->setLevel(join ' ', @active_path);
  if ($active_cfg->isLeafNode()) {
    findDeletedValues($new_ref, \@active_path);
    return;
  }
  # not a leaf node
  my @active_nodes = $active_cfg->listOrigNodesDA();
  foreach (@active_nodes) {
    if (!defined($new_ref->{$_})) {
      my @plist = applySingleQuote(@active_path, $_);
      push @delete_list, [\@plist, 0];
    } else {
      findDeletedNodes($new_ref->{$_}, [ @active_path, $_ ]);
    }
  }
}

my @set_list = ();

# find specified node's values in active config that are set
# (added or changed).
# $0: hash ref at the current hierarchy level (new config)
# $1: array ref representing current config path (active config)
sub findSetValues {
  my $new_ref = $_[0];
  my @active_path = @{$_[1]};

  $active_cfg->setLevel(join ' ', @active_path);
  my ($is_multi, $is_text) = $active_cfg->parseTmpl();
  if ($is_multi) {
    # for "multi:" nodes, need to sort the values by the original order.
    my @nvals = getSortedMultiValues($new_ref, \@active_path);
    if ($is_text) {
      @nvals = map { /^"(.*)"$/ ? $1 : $_ } @nvals;
    }
    my @ovals = $active_cfg->returnOrigValues('');
    my %comp_hash = $active_cfg->compareValueLists(\@ovals, \@nvals);
    foreach (@{$comp_hash{'added'}}) {
      my @plist = applySingleQuote(@active_path, $_);
      push @set_list, [\@plist, 0];
    }
  } else {
    my @nvals = keys %{$new_ref};
    my $nval = $nvals[0];
    if ($is_text) {
      $nval =~ s/^"(.*)"$/$1/;
    }
    my $oval = $active_cfg->returnOrigValue('');
    if (!defined($oval) || ($nval ne $oval)) {
      my @plist = applySingleQuote(@active_path, $nval);
      push @set_list, [\@plist, 0];
    }
  }
}

# find nodes in new config that are set (added or changed).
# $0: hash ref at the current hierarchy level (new config)
# $1: array ref representing current config path (active config)
sub findSetNodes {
  my $new_ref = $_[0];
  my @active_path = @{$_[1]};
  $active_cfg->setLevel(join ' ', @active_path);
  my @active_nodes = $active_cfg->listOrigNodes();
  my %active_hash = map { $_ => 1 } @active_nodes;
  my $nref = $active_cfg->parseTmplAll();
  if (defined($nref->{type}) and !defined($nref->{tag})) {
      # we are at a leaf node.
      findSetValues($new_ref, \@active_path);
      return;
  }
  foreach (sort keys %{$new_ref}) {
      if (scalar(keys %{$new_ref->{$_}}) == 0) {
	  # we are at a non-value leaf node.
	  # check if we need to add this node.
	  if (!defined($active_hash{$_})) {
	      my @plist = applySingleQuote(@active_path, $_);
	      push @set_list, [\@plist, 0];
	  } else {
	      # node already present. do nothing.
	  }
	  next;
      }
      # we recur regardless of whether it's in active. all changes will be
      # handled when we reach leaf nodes (above).
      findSetNodes($new_ref->{$_}, [ @active_path, $_ ]);
  }
}

# compare the current active config with the specified hierarchy and return
# the "diff".
# $0: hash ref of config hierarchy.
# return: hash containing the diff.
sub getConfigDiff {
  $active_cfg = new Vyatta::Config;
  $new_cfg_ref = shift;
  @set_list = ();
#  @disable_list = ();
  @delete_list = ();
  findDeletedNodes($new_cfg_ref, [ ]);
  findSetNodes($new_cfg_ref, [ ]);

  # need to filter out deletions of nodes with default values
  my @new_delete_list = ();
  foreach my $del (@delete_list) {
    my @comps = map { 
	my $file = $_;
	$file =~ s/^'(.*)'$/$1/; 
	$file; 
    } @{${$del}[0]};

    $active_cfg->setLevel(join ' ', @comps);
    my ($is_multi, $is_text, $default) = $active_cfg->parseTmpl();
    if (!defined($default)) {
      push @new_delete_list, $del;
    }
  }

  #let's get active disable nodes
  my @enable_list = $active_cfg->getAllDeactivated("");;

  my %diff = (
              'delete' => \@new_delete_list,
              'set' => \@set_list,
              'deactivate' => \@disable_list,
              'activate' => \@enable_list,
              'comment' => \@comment_list,
             );
  return %diff;
}

1;
