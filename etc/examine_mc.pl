#!/usr/bin/perl -w
#
use strict;
use Getopt::Std;

#------------------------------------------------------------------------------
#
# Usage:
#
# examine_mc.pl -f testset.mc 
#
#------------------------------------------------------------------------------

use vars qw/ $opt_f /;

getopts('f:');

my $file = $opt_f || 0;

#------------------------------------------------------------------------------

# All classifier is binary from 0..n

my %scores_count;
my %scores_right;
my %classifiers_count;
my %classifiers_right;

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

  if ( $line =~ /(.*) \[ (.*) \] \{ (.*) \}/ ) {

    $target = $1;

    $classifications = $2;
    @c_parts  = split (/ /, $classifications);
    for ( my $i = 0; $i <= $#c_parts;  $i += 4 ) { # 4 values per classifier
      my $cfr = sprintf("C#%01i", $i/4);
      if ( $c_parts[$i] eq $target ) {
	print "1";
	$binary .= "1";
	++$classifiers_right{$cfr};
      } else {
	print "0";
	$binary .= "0";
      }
      ++$classifiers_count{$cfr};
    }

    $best = $3; # merged distribution
    @b_parts  = split (/ /, $best);
    $best_token = $b_parts[0];
    
    # Combined classifier.
    #
    ++$scores_count{$binary};
    if ( $best_token eq $target ) {
      print " 1";
      ++$scores_right{$binary};
    } else {
      print " 0";
    }
  } elsif ( $line =~ /(.*) \[ (.*) \]/ ) {
    
    $target = $1;
    
    $classifications = $2;
    @c_parts  = split (/ /, $classifications);
    for ( my $i = 0; $i <= $#c_parts;  $i += 4 ) {
      my $cfr = sprintf("C#%01i", $i/4);
      ++$classifiers_count{$cfr};
      if ( $c_parts[$i] eq $target ) {
	print "1";
	++$classifiers_right{$cfr};
      } else {
	print "0";
	$binary .= "0";
      }
    }
  }
  print " $target\n";
}

foreach my $key (sort (keys(%scores_count))) {
  print "all classifiers: $key: combined correct:";
  if ( defined $scores_right{$key} ) {
    print $scores_right{$key};
  } else {
    print "0";
  }
  print "/$scores_count{$key}";
  print "\n";
}
foreach my $key (sort (keys(%classifiers_count))) {
  print "$key: ";
  if ( defined $classifiers_right{$key} ) {
    print $classifiers_right{$key};
  } else {
    print "0";
  }
  print "/$classifiers_count{$key}";
  print "\n";
}


#11: 950/950
#01: 613/51
#00: 16490/69
#10: 4776/4415

