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

  chomp $line;

  my @parts  = split (/ /, $line);
  if ( $#parts < 4 ) {
    next;
  }

  if ( $parts[6] > 100 ) { # converto to % first
    my $t = $parts[6]+$parts[7]+$parts[8];
    $parts[6] = $parts[6]*100/$t;
    $parts[7] = $parts[7]*100/$t;
    $parts[8] = $parts[8]*100/$t;

    $t = $parts[10]+$parts[11]+$parts[12];
    $parts[10] = $parts[10]*100/$t;
    $parts[11] = $parts[11]*100/$t;
    $parts[12] = $parts[12]*100/$t;
  }
  
  my $pdiff = 0;
  if ( $parts[6] > 0 ) {
    $pdiff = sprintf( "%4.2f", ($parts[10]-$parts[6])*100/$parts[6] );
  }
  print "\\num{".$parts[1]."} & \\cmp{".$parts[2]."} & ";
  print "\\num{".sprintf( "%4.2f", $parts[6])."} & \\num{".sprintf( "%4.2f", $parts[10])."} & ";
  print "\\num{".$pdiff."} \\\\ \n";

}
