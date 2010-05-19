#!/usr/bin/perl -w
#
use strict;
use Getopt::Std;

#------------------------------------------------------------------------------
#
# Usage:
#
# examine_mg.pl -f testset.mg
#
# Maybe read kvs file?
#
#------------------------------------------------------------------------------

use vars qw/ $opt_f /;

getopts('f:');

my $file = $opt_f || 0;

#------------------------------------------------------------------------------

# All classifier is binary from 0..n

my %classifiers_count;
my %classifiers_score;

open(FH, $file) || die "Can't open file.";

while ( my $line = <FH> ) {

  if ( substr($line, 0, 1) eq "#" ) {
    next;
  }

  chomp $line;
  
  my $instance;
  my $classifications;
  my $target;
  my $best;
  my $best_token;
  my $binary = "";
  my (@i_parts, @c_parts, @b_parts);

  # she should eat [ dflt 1 be 0 D ic 1 0 ] 205 3246 { be 762 not 492 }
  #
  if ( $line =~ /(.*) \[ (.*) \] (.*) (.*) \{ (.*)\}/ ) {

    my $instance        = $1;
    my $classifier_info = $2;
    my $distr_size      = $3;
    my $dist_sumfreq    = $4;
    my $distribution    = $5;

    @c_parts  = split (/ /, $classifier_info);
    my $c_name  = $c_parts[0];
    my $c_fco   = $c_parts[1];
    my $c_class = $c_parts[2];
    my $c_prob  = $c_parts[3];
    my $c_type  = $c_parts[4];
    my $c_info  = $c_parts[5];
    my $c_md    = $c_parts[6];
    my $c_mal   = $c_parts[7];

    ++$classifiers_count{$c_name};
    ++$classifiers_score{$c_name}{$c_info};
  }
}

my @infos = ('cg', 'cd', 'ic');
foreach my $c_name (keys(%classifiers_score)) {
  print "$c_name: $classifiers_count{$c_name} ";
  foreach my $c_info ( @infos ) {
  #foreach my $c_info (sort (keys( %{$classifiers_score{$c_name}} ))) {
    print "$c_info:";
    if ( defined $classifiers_score{$c_name}{$c_info} ) {
      print $classifiers_score{$c_name}{$c_info};
    } else {
      print "0";
    }
    print " ";
  }
  print "\n";
}
