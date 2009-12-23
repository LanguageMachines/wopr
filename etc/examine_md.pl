#!/usr/bin/perl -w
#
use strict;
use Getopt::Std;

#------------------------------------------------------------------------------
#
# Usage:
#
# examine_md.pl -f testset.md 
#
# Input:
#  one girl will break [ move 0.121212 , 0.0534477 ] { move 0.121458 }
#  girl will break to [ the 0.127706 , 0.0534477 ] { the 0.173267 }
#  will break to humiliate [ them 1 , 0.0534477 ] { them 1.00112 }
#
# Output (0 for wrong, 1 for right, target):
#  00 0 break
#  00 0 to
#  00 0 humiliate
#  ...
#  00: 20/0         both wrong/combined_correct
#  01: 1/0          second right/combined_correct
#  10: 4/3          first right ...
#  11: 1/1          both right ...
#
# Output for md2 file:
#  00 Cady's        (0=wrong, 1=right) for each classiier,
#  00 passion       followed by target
#  10 for
#  ...
#  0: 7/1           classifier 0: 1 right out of 7
#  1: 7/0           classifier 1: 0 right out of 7
#
#------------------------------------------------------------------------------

use vars qw/ $opt_f /;

getopts('f:');

my $file = $opt_f || 0;

#------------------------------------------------------------------------------

# All classifiers is binary from 0..n

my %scores_count;
my %scores_right;

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

    $instance = $1;
    @i_parts  = split (/ /, $instance);
    $target = $i_parts[$#i_parts];

    $classifications = $2;
    @c_parts  = split (/ /, $classifications);
    for ( my $i = 0; $i <= $#c_parts;  $i += 4 ) {
      if ( $c_parts[$i] eq $target ) {
	print "1";
	$binary .= "1";
      } else {
	print "0";
	$binary .= "0";
      }
    }

    $best = $3;
    @b_parts  = split (/ /, $best);
    $best_token = $b_parts[0];
    
    ++$scores_count{$binary};
    if ( $best_token eq $target ) {
      print " 1";
      ++$scores_right{$binary};
    } else {
      print " 0";
    }
  } elsif ( $line =~ /(.*) \[ (.*) \]/ ) {
    
    $instance = $1;
    @i_parts  = split (/ /, $instance);
    $target = $i_parts[$#i_parts];
    
    $classifications = $2;
    @c_parts  = split (/ /, $classifications);
    for ( my $i = 0; $i <= $#c_parts;  $i += 2 ) {
      ++$scores_count{$i/4};
      if ( $c_parts[$i] eq $target ) {
	print "1";
	++$scores_right{$i/2};
      } else {
	print "0";
	$binary .= "0";
      }
    }
  }
  print " $target\n";
}

foreach my $key (sort (keys(%scores_count))) {
  print "$key: $scores_count{$key}/";
  if ( defined $scores_right{$key} ) {
    print $scores_right{$key};
  } else {
    print "0";
  }
  print "\n";
}


#11: 950/950
#01: 613/51
#00: 16490/69
#10: 4776/4415

