#!/usr/bin/perl -w
# $Id: $
#
use strict;
use Getopt::Std;

#------------------------------------------------------------------------------
# Takes a -w output file from pplx_px.pl and a (corresponding) 
# lexicon file. Creates a gnuplot file to plot a scatter graph
# for correct-guess-percentage against frequency.
#------------------------------------------------------------------------------

use vars qw/ $opt_w $opt_l /;

getopts('w:l:');

my $ws_file  = $opt_w || 0; #file from "pplx_pl.pl -w"
my $lex_file = $opt_l || 0; #lexicon file, lines of: word freq

#------------------------------------------------------------------------------

my %lex;

open(FHW, $lex_file)  || die "Can't open file.";
while ( my $line = <FHW> ) {
  chomp $line;

  my @parts  = split(/ /, $line);

  $lex{$parts[0]} = $parts[1];
}
close(FHW);

my $data_file = $ws_file.".data";
open(OFHD, ">$data_file")  || die "Can't open outfile.";

open(FHW, $ws_file)  || die "Can't open file.";
while ( my $line = <FHW> ) {
  if ( substr($line, 0, 1) eq "#" ) {
    next;
  }
  chomp $line;

  if ( $line =~ /(.*?): cg:(.*?) \((.*?)%\) cd:(.*?) \((.*?)%\) ic:(.*?) \((.*?)%\)/ ) {

    my $word = $1;
    my $cg_a = $2; #_absolute
    my $cg_p = $3; #_percentage
    my $cd_a = $4;
    my $cd_p = $5;
    my $ic_a = $6;
    my $ic_p = $7;

    my $lex_f = $lex{$word} || 0;
    if ( $lex_f ) {
      print OFHD "$word $cg_p $cd_p $ic_p $lex_f\n";
    }
  }
  
}
close(FHW);

close(OFHD);

my $plot_file = $ws_file.".plot";
open(OFHD, ">$plot_file")  || die "Can't open outfile.";
print OFHD "set title \"scatter\"\n";
print OFHD "set xlabel \"frequency\"\n";
print OFHD "set logscale x\n";
print OFHD "set key bottom\n";
print OFHD "set ylabel \"cg\"\n";
print OFHD "set grid\n";
print OFHD "plot [][] \"".$data_file."\" using 5:2\n";
print OFHD "set terminal push\n";
print OFHD "set terminal postscript eps enhanced color solid rounded lw 2 'Helvetica' 10\n";
print OFHD "set out '".$ws_file.".ps'\n";
print OFHD "replot\n";
print OFHD "!epstopdf '".$ws_file.".ps'\n";
print OFHD "set term pop\n";

close(OFHD);

# NB, perl log is base e.
#
sub log2tolog10 {
  my $l    = shift;
  my $e    = 2.718281828;
  my $log2 = 0.3010299957;

  return $l * $log2;
}

__END__

ws file:
with: cg:18 (15.25%) cd:73 (61.86%) ic:27 (22.88%)

set title "scatter"
set xlabel "frequency"
set logscale x
set key bottom
set ylabel "cg"
set grid
plot [][] "foo.data" using 5:2
set terminal push
set terminal postscript eps enhanced color solid rounded lw 2 'Helvetica' 10
set out 'NY_pplx_-a4+D.ps'
replot
!epstopdf 'NY_pplx_-a4+D.ps'
set term pop


foo.data:
Chinese 66.67 33.33 0.00 226
