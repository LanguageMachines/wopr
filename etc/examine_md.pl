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
#
#------------------------------------------------------------------------------

use vars qw/ $opt_f /;

getopts('f:');

my $file = $opt_f || 0;

#------------------------------------------------------------------------------

open(FH, $file) || die "Can't open file.";

while ( my $line = <FH> ) {

  if ( substr($line, 0, 1) eq "#" ) {
    next;
  }

  chomp $line;
  #print $line;
  
  my $instance;
  my $classifications;
  my $target;
  my $best;
  my $best_token;
  my (@i_parts, @c_parts, @b_parts);

  if ( $line =~ /(.*) \[ (.*) \] \{ (.*) \}/ ) {

    $instance = $1;
    @i_parts  = split (/ /, $instance);
    $target = $i_parts[$#i_parts];

    $classifications = $2;
    @c_parts  = split (/ /, $classifications);
    for ( my $i = 0; $i <= $#c_parts;  $i += 2 ) {
      if ( $c_parts[$i] eq $target ) {
	print "1";
      } else {
	print "0";
      }
    }

    $best = $3;
    @b_parts  = split (/ /, $best);
    $best_token = $b_parts[0];
    
    if ( $best_token eq $target ) {
      print " 1";
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
      if ( $c_parts[$i] eq $target ) {
	print "1";
      } else {
	print "0";
      }
    }
  }
  print " $target\n";
}

#printf( "correct guess: %8i (%5.2f)\n", $cg, $cg/$total*100 );
#printf( "correct dist.: %8i (%5.2f)\n", $cd, $cd/$total*100 );
#printf( "correct total: %8i (%5.2f)\n", ($cd+$cg), ($cd+$cg)/$total*100 );
#printf( "incorrect    : %8i (%5.2f)\n", $ic, $ic/$total*100 );
#printf( "total guesses: %8i\n", $total );

