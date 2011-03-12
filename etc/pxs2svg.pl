#!/usr/bin/perl -w
#
use strict;
use Getopt::Std;

#------------------------------------------------------------------------------
# User options
#
# perl pxs2svg.pl -f out10.gen.ws3.pxs > ~/foo.svg 
#
#------------------------------------------------------------------------------

use vars qw/ $opt_b $opt_e $opt_f $opt_v $opt_w /;

getopts('b:e:f:vw:');

my $basename   = $opt_b ||  "out";
my $file       = $opt_f ||  0;
my $eos_mode   = $opt_e ||  0;
my $verbose    = $opt_v ||  0;

#------------------------------------------------------------------------------

my $scnt    = 0;
$basename   = $basename.".%04d";
my $svgfile = sprintf( $basename.".svg", $scnt );

open(FH, $file) || die "Can't open file.";
#open(PFH, ">$svgfile") || die "Can't open plotfile.";

#viewBox="0 0 800 1200"
print <<EOF;
<?xml version="1.0" encoding="utf-8"  standalone="no"?>
<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN" 
 "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">
<svg width="100%" height="800%"
 xmlns="http://www.w3.org/2000/svg"
 xmlns:xlink="http://www.w3.org/1999/xlink">
EOF

my $y = 200;

while ( my $line = <FH> ) {
  #print $line;
  if ( substr($line, 0, 1) eq "#" ) {
    next;
  }

  chomp $line;

  $line =~ m/\[ (.*) \]/;
  my $pplxs = $1;

  my @parts  = split (/ /, $pplxs);
  if ( $#parts < 4 ) {
    next;
  }

  my $x = 150;

  for ( my $i = 0; $i <= $#parts; $i++ ) {
    my $pplx = $parts[$i];
    my $r = sqrt( ($pplx / 3.14156) );
    #print "$pplx/$r ";
    my $colour = "green";
    if ( $r < 15 ) {
      $r = $r * 10;
      $colour = "red";
      
    }
    if ( $r > 150 ) {
      $r = 150;
      $colour = "blue";
    }
    $r = int( $r );
    $x = $x + $r;
    print "<circle cx='$x' cy='$y' r='$r' fill='$colour' stroke='black' width='1' />\n";
    $x = $x + $r;
  }

  print "\n";
  $y = $y + 300;

}

print "</svg>\n";

