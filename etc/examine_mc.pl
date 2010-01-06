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
      if ( $c_parts[$i] eq $target ) {
	print "1";
	$binary .= "1";
      } else {
	print "0";
	$binary .= "0";
      }
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
      ++$scores_count{$i/4};
      if ( $c_parts[$i] eq $target ) {
	print "1";
	++$scores_right{$i/4};
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

