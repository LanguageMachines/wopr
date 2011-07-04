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
# GC28000 10000 l2r0 11.41 22.21 66.38 1 200 -a4+D 1-500 0 449.33 449.33 0.265 0.515

  chomp $line;

  my @parts  = split (/ /, $line);
  if ( $#parts < 4 ) {
    next;
  }

  my $cg = 6; #WEX format
  if ( $#parts < 15 ) {
    $cg = 3; #GC format
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
  
  print "%".$line."\n";

  my $pdiff = 0;
  if ( $parts[$cg] > 0 ) {
    $pdiff = sprintf( "%4.2f", ($parts[$cg+4]-$parts[$cg])*100/$parts[$cg] );
  }
  print "\\num{".$parts[1]."} ";     #lines
  print "& \\cmp{".$parts[2]."} ";   #context
  if ( $cg == 6 ) {
    print "& \\num{".sprintf( "%4.2f", $parts[$cg])."} & \\num{".sprintf( "%4.2f", $parts[$cg+4])."} & ";
    print "\\num{".$pdiff."} ";
  }
  if ( $cg == 3 ) { 
    print "& \\num{".sprintf( "%4.2f", $parts[$cg])."} ";     #cg
    print "& \\num{".sprintf( "%4.2f", $parts[$cg+1])."} ";   #cd
    print "& \\num{".sprintf( "%4.2f", $parts[$cg+2])."} ";   #ic
    print "& \\num{".sprintf( "%i", $parts[$cg+3])."} ";      #gcs
    print "& \\num{".sprintf( "%i", $parts[$cg+4])."} ";      #gcd
    print "& \\cmp{".sprintf( "%s", $parts[$cg+6])."} ";      #range
    if ( $#parts > 10 ) {
      print "& \\cmp{".sprintf( "%s", $parts[$cg+8])."} ";      #pplx
      print "& \\cmp{".sprintf( "%s", $parts[$cg+10])."} ";     #mmr(cd)
    }
  }
  print "\\\\ \n"; #% ".$parts[0]."\n";

}
