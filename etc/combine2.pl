#!/usr/bin/perl -w
#
use strict;
use Getopt::Std;
#
#------------------------------------------------------------------------------
# Combine two outputs from data2latex.pl into one dataset,
# to compare for example the cg columns from two different
# experiments.
# perl /data2latex.pl -f DATA.ALG.20000.plot.l2r0 |egrep -v "\-k" |egrep -v "%"
# \num{1000} & \num{16.0} & \num{28.2} & \num{55.8} & \num{156} & \num{0.182} & \cmp{l2r0 -a1+D} \\ 
#------------------------------------------------------------------------------

use vars qw/ $opt_f $opt_g /;

getopts('f:g:');

my $file0 = $opt_f ||  0;
my $file1 = $opt_g ||  0;

open(FH0, $file0) || die "Can't open file.";
open(FH1, $file1) || die "Can't open file.";

my $var0 = 2; # cg
my $fmt0 = "%.1f";
my $var1 = 4; # cd
my $fmt1 = "%.1f";
#my $var0 = 8; # pplx
#my $fmt0 = "%.0f";
#my $var1 = 10; # mrr
#my $fmt1 = "%.3f";

while ( my $line0 = <FH0> ) {
   my $line1 = <FH1>;
   if ( defined($line1) ) {

    chomp $line0;
    chomp $line1;

    my @p0 = split (/ /, $line0);
    my @p1 = split (/ /, $line1);

    if ( $#p0 < 8 ) {
      next;
    }

    if ( defined($p0[$var0]) ) {
      print "$p0[0] & ";
      print "$p0[$var0] & $p1[$var0] & ";
      my ( $p0_cg ) = $p0[$var0] =~ m{(\d+\.?\d+)};
      my ( $p1_cg ) = $p1[$var0] =~ m{(\d+\.?\d+)};
      print "\\num{". sprintf( $fmt0, ($p1_cg-$p0_cg)) ."} & ";
      print "$p0[$var1] & $p1[$var1] & ";
      my ( $p0_cd ) = $p0[$var1] =~ m{(\d+\.?\d+)};
      my ( $p1_cd ) = $p1[$var1] =~ m{(\d+\.?\d+)};
      print "\\num{". sprintf( $fmt1, ($p1_cd-$p0_cd)) ."} & ";
      print "$p0[12] $p0[13]"; # ctx and algo
      print " \\\\\n";
    }
  }
}

