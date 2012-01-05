#!/usr/bin/perl -w
# $Id: pplx_px.pl 13475 2011-10-27 11:15:11Z pberck $
#
use strict;
use Getopt::Std;
#use Math::BigFloat;
#Math::BigFloat->accuracy(10);

#------------------------------------------------------------------------------
# User options
# ...
#------------------------------------------------------------------------------

use vars qw/ $opt_f /;

getopts('f:');

my $pdt2_file  = $opt_f || 0;

#------------------------------------------------------------------------------

open(FHW, $pdt2_file)  || die "Can't open file.";

my $fix    = 0;
my $fixL   = 0;
my $fixM   = 0;
my $lsaved = 0;
my $adjust = 0;

while ( my $line = <FHW> ) {

  if ( substr($line, 0, 1) eq "#" ) {
    next;
  }
  chomp $line;

  my @parts  = [];
  my @lparts = [];
  @parts  = split (/ /, $line);

  # counters
  @lparts = split (/\./, substr( $parts[0], 1 ) );

  my $num = $#lparts;
  if ( $num == 0 ) {
    #print "New sentence.\n";
  } elsif ( $num == 1 ) {
    #print "New word.\n";
  } elsif ( $num == 2 ) {
    #print "New instance.\n";
  } else {
    #print "Error.\n";
  }

  # L line?
  # L0000.0000 o m m u communication 9
  if ( substr( $line, 0, 1 ) eq 'L' ) {
    if ( $fix == 1 ) {
      print "Adjust.\n";
      ++$adjust;
    }
    $fixL = substr( $parts[0], 1, 9 ); #save nnnn.mmmm
    $fix = 1;
    $lsaved = $parts[ $#parts ];
    print "L $fixL $lsaved\n";
  }
  if ( substr( $line, 0, 1 ) eq 'M' ) {
    $fixM = substr( $parts[0], 1, 9 ); #save nnnn.mmmm
    print "M $fixM\n";
  }

  if ( ($fix == 1) && ($fixL eq $fixM) ) {
    print "cancel.\n";
    $fix = 0;
  }

  if ( substr( $line, 0, 1 ) eq 'R' ) {
    print "Adjust last.\n";
    ++$adjust;
  }

  # The fix is applied when we hit the next counter, or
  # reset when we hit the next M...

  # Totals
  # T0 136754 43831 32.051
  # T1 136754 23980 17.5351
  # T 136754 67811 49.5861

  if ( substr( $line, 0, 1 ) eq 'T' ) {
    # Letter totals
    if ( substr( $line, 0, 2 ) eq 'T0' ) {
      print "Adjust: $adjust\n";
      #print "$line\n";
      print "T0 ".$parts[1]." ".($parts[2]-$adjust)." ".sprintf("%.3f",((($parts[2]-$adjust)/$parts[1])*100))."\n";
    } elsif ( substr( $line, 0, 2 ) eq 'T1' ) {
      print "$line\n";
    } elsif ( substr( $line, 0, 2 ) eq 'T ' ) {
      #print "$line\n";
      print "T ".$parts[1]." ".($parts[2]-$adjust)." ".sprintf("%.3f",((($parts[2]-$adjust)/$parts[1])*100))."\n";
    }

  }
  
} 

close(FHW);

__END__

# l3 5 1 1
# austen.txt.1e4.lt4_-a1+D.ibase austen.txt.1e4.l2r0_-a1+D.ibase
S0000 communication of Mrs Elton and me 33
I0000.0000 _ communication
L0000.0000.0004 o m m u communication 9
P0000.0000.0000 and a very
P0000.0000.0001 of Mrs Elton
M0000.0000.0001 of Mrs Elton 12
P0000.0000.0002 could rest with

