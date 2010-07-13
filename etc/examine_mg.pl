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

use vars qw/ $opt_f $opt_l $opt_r /;

getopts('f:l:r:');

my $file = $opt_f || 0;
my $lc   = $opt_l || 0;
my $rc   = $opt_r || 0;

#------------------------------------------------------------------------------

# All classifier is binary from 0..n

my %classifiers_count;
my %classifiers_score;
my %totals_score;
my %rank_sum;

my $mg_target_pos = $lc + $rc + 0;
my $mg_c_name_pos = $mg_target_pos + 4;
my $mg_c_type_pos = $mg_target_pos + 3;
my $mg_icu_pos    = $lc + $rc + 5;
my $mg_prob_pos   = $lc + $rc + 2;
my $mg_dsize_pos  = $lc + $rc + 9;
my $mg_sumf_pos   = $lc + $rc + 10;
my $mg_rank_pos   = $lc + $rc + 11;

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

  my @parts = split (/ /, $line);

  # _ between themselves or to -5.39232 D dflt cd k 1 0 58 84 4 [ to 6 and 5
  #                                                          into 3 of 3 for 3 ]
  #
  my $c_type     = $parts[ $mg_c_type_pos ];
  my $c_name     = $parts[ $mg_c_name_pos ];
  my $c_info     = $parts[ $mg_icu_pos ];
  my $c_rank     = $parts[ $mg_rank_pos ];

  ++$classifiers_count{$c_name};
  ++$classifiers_score{$c_name}{$c_info};
  ++$totals_score{$c_info};
  $rank_sum{$c_info} += $c_rank;
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

print "\nTotals:\n";
foreach my $c_info ( @infos ) {
  print "$c_info:";
  if ( defined $totals_score{$c_info} ) {
    print $totals_score{$c_info}." ".$rank_sum{$c_info}." ".$rank_sum{$c_info}/$totals_score{$c_info};
  } else {
    print "0";
  }
  print "\n";
}


