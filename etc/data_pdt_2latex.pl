#!/usr/bin/perl -w
#
use strict;
use Getopt::Std;
#
#------------------------------------------------------------------------------
# User options
#------------------------------------------------------------------------------

use vars qw/ $opt_f $opt_v /;

getopts('f:v:');

my $file       = $opt_f || 0;
my $vars       = $opt_v || "cg";

open(FH, $file) || die "Can't open file.";

print "%generated  perl data2latex.pl -f $file\n";
print <<'EOF';
\begin{table}[h!]\caption{Scores}
\centering
\vspace{\baselineskip}
\begin{tabular}{ r r r r r r }
\toprule
Lines & Kp & kps & pc & n & ds \\
\midrule
EOF


while ( my $line = <FH> ) {
  if ( substr($line, 0, 1) eq "#" ) {
    next;
  }

# PRE10008 1000 136754 9851 7.20345 3 555 l3r0_-a1+D
#   or
# PDTT10008 10000 1000 136754 60301 44.0945 51031 37.3159 9270 6.7786 3 3 511 l12_-a1+D l2_-a1+D
#
  chomp $line;

  my @parts  = split (/ /, $line);
  if ( $#parts < 4 ) {
    next;
  }

  my $cg = 6; #WEX format
  if ( $#parts < 15 ) {
    $cg = 3; #GC format
  }
  if ( substr($parts[0], 0, 3) eq "ALG" ) {
    $cg = 2; #ALG format
  }
  if ( substr($parts[0], 0, 4) eq "PDTT" ) {
    $cg = 2; #PDT2 format
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
    $pdiff = sprintf( "%.2f", ($parts[$cg+4]-$parts[$cg])*100/$parts[$cg] );
  }

  print "\\num{".$parts[1]."} ";     #lines
  #print "& \\cmp{".$parts[2]."} ";   #context

  if ( $cg == 6 ) {
    print "& \\num{".sprintf( "%.2f", $parts[$cg])."} & \\num{".sprintf( "%.2f", $parts[$cg+4])."} & ";
    print "\\num{".$pdiff."} ";
  }
  if ( $cg == 3 ) {  #GC experiments
    print "& \\num{".sprintf( "%.1f", $parts[$cg])."} ";     #cg
    print "& \\num{".sprintf( "%.1f", $parts[$cg+1])."} ";   #cd
    print "& \\num{".sprintf( "%.1f", $parts[$cg+2])."} ";   #ic
    print "& \\num{".sprintf( "%i", $parts[$cg+3])."} ";     #gcs
    print "& \\num{".sprintf( "%i", $parts[$cg+4])."} ";     #gcd
    print "& \\cmp{".sprintf( "%s", $parts[$cg+6])."} ";     #range
    if ( $#parts > 10 ) {
      print "& \\num{".sprintf( "%.0f", $parts[$cg+8])."} ";   #pplx
      print "& \\num{".sprintf( "%.3f", $parts[$cg+10])."} ";  #mmr(cd)
    } else {
      print "& \\num{0} ";   #pplx
      print "& \\num{0} ";   #mmr(cd)
    }
  }
  if ( $cg == 2 ) { # ALG
    if ( $vars eq "cg" ) {
      print "& \\num{".sprintf( "%.1f", $parts[$cg])."} ";     #cg
      print "& \\num{".sprintf( "%.1f", $parts[$cg+1])."} ";   #cd
      print "& \\num{".sprintf( "%.1f", $parts[$cg+2])."} ";   #ic
      print "& \\num{".sprintf( "%.0f", $parts[$cg+3])."} ";   #pplx
      print "& \\num{".sprintf( "%.3f", $parts[$cg+5])."} ";   #mmr(cd)
    }
    if ( $vars eq "ads" ) {
      print "& \\num{".sprintf( "%.0f", $parts[$cg+8])."} ";   #adc
      print "& \\num{".sprintf( "%.0f", $parts[$cg+9])."} ";   #ads(umfreq)
    }
    (my $timbl = $parts[$cg+10]) =~ tr/_/ /;
    print "& \\cmp{".sprintf( "%s", $timbl)."} ";  #timbl
  }
  print "\\\\ \n"; #% ".$parts[0]."\n";

}

print <<'EOF';
\bottomrule
\end{tabular}
\label{tab:scoresexperiment2}
\end{table}
EOF
