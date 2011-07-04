#!/usr/bin/perl -w
#
use strict;
use Getopt::Std;
#
#------------------------------------------------------------------------------
# User options
#------------------------------------------------------------------------------

use vars qw/ $opt_f /;

getopts('f:');

my $file       = $opt_f ||  0;

open(FH, $file) || die "Can't open file.";
while ( my $line = <FH> ) {
  if ( substr($line, 0, 1) eq "#" ) {
    next;
  }

# WEX10101 10000 l2r1 2 1 cg 17.53 27.29 55.16 0.57 18.82 26.82 54.35 0.70 -a1+D -a4+D 1-200
#   or
# GC26000 10000 l0r1 13.73 25.09 61.18 1 100 -a4+D 200-250 0

  chomp $line;

  my @parts  = split (/ /, $line);
  if ( $#parts < 4 ) {
    next;
  }

  my $cg = 6;
  if ( $#parts < 12 ) {
    $cg = 3;
  }

  if ( $parts[$cg] > 100 ) { # converto to % first
    my $t = $parts[$cg]+$parts[7]+$parts[8];
    $parts[$cg] = $parts[$cg]*100/$t;
    $parts[$cg+1] = $parts[$cg+1]*100/$t;
    $parts[$cg+2] = $parts[$cg+2]*100/$t;

    $t = $parts[$cg+4]+$parts[$cg+5]+$parts[$cg+6];
    $parts[$cg+4] = $parts[$cg+4]*100/$t;
    $parts[$cg+5] = $parts[$cg+5]*100/$t;
    $parts[$cg+6] = $parts[$cg+6]*100/$t;
  }
  
  my $pdiff = 0;
  if ( $parts[$cg] > 0 ) {
    $pdiff = sprintf( "%4.2f", ($parts[$cg+4]-$parts[$cg])*100/$parts[$cg] );
  }
  print "\\num{".$parts[1]."} & \\cmp{".$parts[2]."} & ";
  if ( $cg == 6 ) {
    print "\\num{".sprintf( "%4.2f", $parts[$cg])."} & \\num{".sprintf( "%4.2f", $parts[$cg+4])."} & ";
    print "\\num{".$pdiff."} \\\\ \n";
  }
  if ( $cg == 3 ) { #cg, cg, ic, gcs, gcd, range
    print "\\num{".sprintf( "%4.2f", $parts[$cg])."} & \\num{".sprintf( "%4.2f", $parts[$cg+1])."} & \\num{".sprintf( "%4.2f", $parts[$cg+2])."} & ";
    print "\\num{".sprintf( "%i", $parts[$cg+3])."} & \\num{".sprintf( "%i", $parts[$cg+4])."} & \\num{".sprintf( "%s", $parts[$cg+6])."} ";
    print "\\\\ \n";
  }


}
