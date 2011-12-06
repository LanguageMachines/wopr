#!/usr/bin/perl -w
# $Id: $
#
use strict;
use Getopt::Std;

#------------------------------------------------------------------------------
# 
# Generates a file for gnuplot with three columns:
#   index sumlog2prob-file0 sumlog2prob-file1
#'
# perl ../../etc/create_pxs_scatterplot.pl  -f en-es.es.t1000.l2r0_TRIBL2.pxs -F en-es.es.t1000.l2r1_TRIBL2.pxs -o tmp3
# gnuplot> plot [][] "tmp3" using 2:3
#
# awk '{print $3;}' en-es.es.t1000.l2r0_TRIBL2.pxs is easier...
#
#------------------------------------------------------------------------------

use vars qw/ $opt_f $opt_F $opt_l $opt_L $opt_r $opt_R $opt_o /;

getopts('f:F:l:L:r:R:o:');

my $file0 = $opt_f || 0;
my $file1 = $opt_F || 0;

my $lc0   = $opt_l || 0;
my $lc1   = $opt_L || 0;
my $rc0   = $opt_r || 0;
my $rc1   = $opt_R || 0;

my $out   = $opt_o || 0;

#------------------------------------------------------------------------------

my $gcs = 0;

my $l2p_pos0 = 2;
my $l2p_pos1 = 2;

my $out_data = $out.".data";
my $out_plot = $out.".plot";
# $out.ps and $out.pdf generated by script

# Data file first
#
open(FHF0, $file0)  || die "Can't open file.";
open(FHF1, $file1)  || die "Can't open file.";
open(OFHD, ">$out_data") || die "Can't open datafile.";

print OFHD "# $file0\n";
print OFHD "# $file1\n";

my $cnt = 0;
while ( my $line0 = get_next(*FHF0) ) {

  my $l2p0;
  my $l2p1;
  my @parts0;
  my @parts1;

  if ( my $line1 = get_next(*FHF1) ) {

    chomp $line0;
    chomp $line1;

    @parts0 = split ( / /, $line0 );
    $l2p0 = $parts0[$l2p_pos0];

    @parts1 = split ( / /, $line1 );
    $l2p1 = $parts1[$l2p_pos1];

    print OFHD "$cnt $l2p0 $l2p1\n";
    ++$cnt;
    
  }
}

close(OFHD);
close(FHF1);
close(FHF0);

# Gnuplot file
#
open(OFHP, ">$out_plot") || die "Can't open plotfile.";

print OFHP "# $file0\n";
print OFHP "# $file1\n";
print OFHP "set xrange []\n";
print OFHP "set yrange []\n";
print OFHP "set xlabel '$file0'\n";
print OFHP "set ylabel '$file1'\n";
print OFHP "set key bottom\n";
print OFHP "set grid\n";
print OFHP "plot \"$out_data\" using 2:3\n";
print OFHP "set terminal push\n";
print OFHP "set terminal postscript eps color lw 2 \"Helvetica\" 10\n";
print OFHP "set out \"$out.ps\"\n";
print OFHP "replot\n";
print OFHP "!epstopdf '".$out.".ps'\n";
print OFHP "set term pop\n";

close(OFHP);

# ----

sub get_next {
 my $h = shift;
 while ( my $line = <$h> ) {
   if ( substr($line, 0, 1) eq "#" ) {
     return get_next($h);
   }
   chomp $line;
   return $line;
 }
 return 0;
}

__END__

