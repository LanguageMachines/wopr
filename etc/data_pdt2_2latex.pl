#!/usr/bin/perl -w
#
use strict;
use Getopt::Std;
#
#------------------------------------------------------------------------------
# User options
# PJB: better to select data to PRE format, and plot that with the
# script to create plots fpr the PRE data...?
#
#------------------------------------------------------------------------------

use vars qw/ $opt_f $opt_v /;

getopts('f:v:');

my $file       = $opt_f || 0;
my $vars       = $opt_v || "cg";

open(FH, $file) || die "Can't open file.";

print "%generated  perl data_pdt2_2latex.pl -f $file\n";
print <<'EOF';
\begin{table}[h!]\caption{Scores}
\centering
\vspace{\baselineskip}
\begin{tabular}{ r r r r r r r r r r }
\toprule
id & L0 & L1 & Kp & kps & pc & dl & n_ds & a0 & a1 \\
\midrule
EOF


while ( my $line = <FH> ) {
  if ( substr($line, 0, 1) eq "#" ) {
    next;
  }

# PDTT10008 10000 1000 136754 60301 44.0945 51031 37.3159 9270 6.7786 3 3 511 l12_-a1+D l2_-a1+D
# 0         1     2    3      4     5       6     7       8    9      
#
  chomp $line;
  $line =~ s/_/\\_/g;

  my @parts  = split (/ /, $line);
  if ( $#parts < 4 ) {
    next;
  }
  print "%".$line."\n";

  # Maybe an index to refer to...
  print $parts[0]." ";

  num( $parts[1], "%i" ); # lines0
  num( $parts[2], "%i" ); # lines1
  num( $parts[3], "%i" ); # keypresses

  num( $parts[4], "%i" ); # saved
  num( $parts[5], "%.1f" ); # saved percentage

  # Select which with a parameter
  #num( $parts[6], "%i" ); # saved
  #num( $parts[7], "%.1f" ); # saved percentage

  #num( $parts[8], "%i" ); # saved
  #num( $parts[9], "%.1f" ); # saved percentage

  num( $parts[10], "%i" ); # depth of letter predictor
  str( "n".$parts[11]."ds".$parts[12] ); # num/depth of word predictor

  str( $parts[13] ); # letter context/algo
  str( $parts[14] ); # word context/algo

  print "\\\\ \n"; #% ".$parts[0]."\n";

}

print <<'EOF';
\bottomrule
\end{tabular}
\label{tab:scoresexperiment2}
\end{table}
EOF

sub num {
  my $n = shift;
  my $d = shift;

  print "& \\num{".sprintf( $d, $n)."} ";
}

sub str {
  my $n = shift;
  my $d = "%s";

  print "& ".sprintf( $d, $n)." ";
}
