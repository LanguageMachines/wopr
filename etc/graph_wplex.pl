#!/usr/bin/perl -w
#
use strict;
use Getopt::Std;

#------------------------------------------------------------------------------
# User options
#
# graph_wplex.pl -f testset.txt.ws3.hpx5.ib_1e6.px -b testtest -e 1
#
# Generates an "out.nnnn.txt" and an "out.nnnn.plot" file per sentence.
# The "out" prefix can be changed with the "-b" option.
#
# Set the eos_mode to 0: for "_ _ _" style files
#                     1: for scanning of .!?
#
# TODO: two separate checks for end-of-sentence is ugly, should be improved.
#
#------------------------------------------------------------------------------

use vars qw/ $opt_b $opt_e $opt_f $opt_v $opt_w /;

getopts('b:e:f:vw:');

my $basename   = $opt_b ||  "out";
my $file       = $opt_f ||  0;
my $eos_mode   = $opt_e ||  0;
my $verbose    = $opt_v ||  0;
my $ws         = $opt_w ||  3;

#------------------------------------------------------------------------------

my $div  = "_ _ _ _ _ _ _ _ _ _";
my $wcnt =  0;
my $scnt = -1;
my $outfile      = sprintf( $basename.".txt", $scnt );
my $plotfile     = sprintf( $basename.".plot", $scnt );
my $plotfname    = sprintf( $basename.".svg", $scnt );
my $xtics_font   = "set xtics font \"Arial, 8\"";
my $xtics_offset = "set xtics offset character -0.8, 0";
my $wrds = "";
my $xtcs = "";
#my $eos_mode = 1; #end of sentence mode (0 = "_ _ _")

$basename  = $basename.".%04d";

open(FH, $file) || die "Can't open file.";
open(OFH, ">$outfile")  || die "Can't open outfile.";
open(PFH, ">$plotfile") || die "Can't open plotfile.";

while ( my $line = <FH> ) {
  #print $line;
  if ( substr($line, 0, 1) eq "#" ) {
    next;
  }

  chomp $line;

  # If we find a "_ _ _ ..." we shave started a new sentence, and we need 
  # to close the current file, and start a new one.
  #
  if ( ($eos_mode == 0) && (substr( $line, 0, $ws*2 ) eq substr( $div, 0, $ws*2)) ) {
    print PFH "set terminal svg dynamic fsize 10\n";
    print PFH "set output \"".$plotfname."\"\n";
    print PFH "set title \"".$outfile."\"\n";
    print PFH "set xtics 1,1 rotate by -90\n";
    chop $xtcs;
    print PFH "$xtics_font\n";
    print PFH "$xtics_offset\n";
    print PFH "set xtics (".$xtcs.")\n";
    print PFH "set logscale y\n";
    print PFH "set yrange []\n";
    print PFH "set y2range [0:5000]\n";
    print PFH "set ytics nomirror\n";
    print PFH "set y2tics 0,1000\n";
    print PFH "set my2tics 10\n";
    print PFH "plot [-1:".$wcnt."][] \"".$outfile."\" using 1:3 t \"wplex\" with lines, \"".$outfile."\" using 1:4 t \"dsize\" with lines\n";
    $scnt++;
    $wcnt = 0;
    $wrds = "";
    $xtcs = "";
    close( OFH ) || die "Error?";
    close( PFH ) || die "Error?";
    $outfile  = sprintf( $basename.".txt", $scnt );
    $plotfile = sprintf( $basename.".plot", $scnt );
    $plotfname = sprintf( $basename.".svg", $scnt );
    open(OFH, ">$outfile") || die "Can't open outfile.";
    open(PFH, ">$plotfile") || die "Can't open plotfile.";
  }

  my @parts  = split (/ /, $line);
  if ( $#parts < 4 ) {
    next;
  }

  my $trgt = $parts[$ws];
  $wrds = $wrds.$trgt." ";
  if ( ($trgt eq "\"") || ($trgt eq "'") ) {
    $xtcs = $xtcs."\"\\".$trgt."\" ".$wcnt.",";
  } else {
    $xtcs = $xtcs."\"".$trgt."\" ".$wcnt.",";
  }

  # 0,1,2: context
  # 3    : target
  # 4    : prediction
  # 5    : logprob
  # 6    : entropy
  # 7    : word pplx
  # 8    : size of distr.
  #
  print OFH $wcnt++."\t".$parts[$ws+2]."\t".$parts[$ws+4]."\t".$parts[$ws+5]."\t".$parts[$ws]."\n";

  if ( ($eos_mode > 0) && ($trgt =~ m/[.?!]/) ) {
    #print "\n";
    print PFH "set terminal svg dynamic fsize 10\n";
    print PFH "set output \"".$plotfname."\"\n";
    print PFH "set title \"".$outfile."\"\n";
    print PFH "set xtics 1,1 rotate by -90\n";
    chop $xtcs;
    print PFH "$xtics_font\n";
    print PFH "$xtics_offset\n";
    print PFH "set xtics (".$xtcs.")\n";
    print PFH "set logscale y\n";
    print PFH "set yrange []\n";
    print PFH "set y2range [0:5000]\n";
    print PFH "set ytics nomirror\n";
    print PFH "set y2tics 0,1000\n";
    print PFH "set my2tics 10\n";
    print PFH "plot [-1:".$wcnt."][] \"".$outfile."\" using 1:3 t \"wplex\" with lines, \"".$outfile."\" using 1:4 t \"dsize\" with lines\n";
    $scnt++;
    $wcnt = 0;
    $wrds = "";
    $xtcs = "";
    close( OFH ) || die "Error?";
    close( PFH ) || die "Error?";
    $outfile  = sprintf( $basename.".txt", $scnt );
    $plotfile = sprintf( $basename.".plot", $scnt );
    $plotfname = sprintf( $basename.".svg", $scnt );
    open(OFH, ">$outfile") || die "Can't open outfile.";
    open(PFH, ">$plotfile") || die "Can't open plotfile.";
  }

}

# Print left over stuff
#
print PFH "set terminal svg dynamic fsize 10\n";
print PFH "set output \"".$plotfname."\"\n";
print PFH "set title \"".$outfile."\"\n";
print PFH "set xtics 1,1 rotate by -90\n";
chop $xtcs;
print PFH "$xtics_font\n";
print PFH "$xtics_offset\n";
print PFH "set xtics (".$xtcs.")\n";
print PFH "set logscale y\n";
print PFH "set yrange []\n";
print PFH "set y2range [0:5000]\n";
print PFH "set ytics nomirror\n";
print PFH "set y2tics 0,1000\n";
print PFH "set my2tics 10\n";
print PFH "plot [-1:".$wcnt."][] \"".$outfile."\" using 1:3 t \"wplex\" with lines, \"".$outfile."\" using 1:4 t \"dsize\" with lines\n";
close( PFH ) || die "Error?";

close(OFH) || die "Error?";
close(FH) || die "Error?";
