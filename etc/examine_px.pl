#!/usr/bin/perl -w
#
use strict;
use Getopt::Std;

#------------------------------------------------------------------------------
# User options
#
# examine_px.pl -f testset.txt.ws3.hpx5.ib_1e6.px -e 1
#
# Set the eos_mode ("-e n") to 0: for "_ _ _" style files
#                              1: for scanning of .!?
#
# TODO: two separate checks for end-of-sentence is ugly, should be improved.
#
# Changed ws paramater to l and r.
# ws3 === l3, r0
#
#------------------------------------------------------------------------------

use vars qw/ $opt_b $opt_e $opt_f $opt_g $opt_l $opt_r $opt_v $opt_w /;

getopts('b:e:f:g:l:r:v:w:');

my $basename   = $opt_b || "out";
my $file       = $opt_f || 0;
my $eos_mode   = $opt_e || 0;
my $var        = $opt_v || "lp"; #wlp, dp, lp, sz, md
my $lc         = $opt_l || 0;
my $rc         = $opt_r || 0;
my $gcs        = $opt_g || 0; #for global context

#------------------------------------------------------------------------------

my $ws = $lc + $rc + $gcs;

my $div  = "_ _ _ _ _ _ _ _ _ _";
my $wcnt =  0;
my $scnt = -1;
my $distr_entropy = 0;
my $word_entropy = 0;
my $var_summed = 0;
my $correct = "";
my $cg = 0;
my $cd = 0;
my $ic = 0;
#--
my $p0;
my $p1;
my $p2;
my $p3;
my $p4;
format  = 
@##: @<<<<<<<<<<<<<<<< @<<<<<<<<<<<<<<<< @###### @######.######
$p0, $p1, $p2, $p3, $p4
.
#--

open(FH, $file) || die "Can't open file.";

while ( my $line = <FH> ) {
  #print $line;
  if ( substr($line, 0, 1) eq "#" ) {
    next;
  }

  chomp $line;

  # If we find a "_ _ _ ..." we have started a new sentence...
  # With only right context, it is the next one...
  #
  if ( ($eos_mode == 0) && (substr( $line, 0, $ws*2 ) eq substr( $div, 0, $ws*2)) ) {
    if ( $wcnt > 0 ) {
      print "sum($var) = $var_summed\n";
      print "avg($var) = ".($var_summed/$wcnt)."\n";
    }
    $wcnt = 0;
    $distr_entropy = 0;
    $word_entropy = 0;
    $var_summed = 0;
  }

  my @parts  = split (/ /, $line);
  if ( $#parts < 4 ) {
    next;
  }

  my $trgt = $parts[$ws];

  #  0,1,2   : context
  #  3 ws    : target
  #  4 ws+1  : prediction
  #  5 ws+2  : logprob of prediction
  #  6 ws+3  : entropy of distro returned (sum (p * log(p)) )
  #  7 ws+4  : word level entropy (2 ** -logprob) (from _word_ logprob)
  #  8 ws+5  : cg/cd/ic
  #  9 ws+6  : md
  # 10 ws+7  : mal
  # 11 ws+8  : size of distro
  #
  $distr_entropy += $parts[$ws+3]; # Sum of all distro-entropies
  $word_entropy += $parts[$ws+4];     # Sum of all WL-entropies
  $correct = $parts[$ws+5];
  my $val = 0.0;
  if ( $var eq "wlp" ) {
    $val = $parts[$ws+4];
  } elsif ( $var eq "dp" ) {
    $val = $parts[$ws+3];
  } elsif ( $var eq "lp" ) {
    $val = $parts[$ws+2];
  } elsif ( $var eq "sz" ) {
    $val = $parts[$ws+8];
  } elsif ( $var eq "md" ) {
    $val = $parts[$ws+6];
  }
  $var_summed += $val;

  if ( $correct eq "cg" ) {
    ++$cg;
  } elsif  ( $correct eq "cd" ) {
    ++$cd;
  } else {
    ++$ic;
  }

  #print $wcnt++."\t".$parts[$ws]."\t".$parts[$ws+1]."\t".$val."\n";

  $p0 = $wcnt++;
  $p1 = $parts[$ws];
  $p2 = $parts[$ws+1];
  $p3 = $parts[$ws+6]; #########################################! was 8
  $p4 = $val;
  write ;

  # If not "_ _ _" we look for end of line punctuation.
  #
  if ( ($eos_mode > 0) && ($trgt =~ m/[.?!]/) ) {
    if ( $wcnt > 0 ) {
      print "sum($var) = $var_summed\n";
      print "avg($var) = ".($var_summed/$wcnt)."\n";
    }
    $wcnt = 0;
    $distr_entropy = 0;
    $word_entropy = 0;
    $var_summed = 0;
  }

}

# Print left over stuff
#
if ( $wcnt > 0 ) {
  print "sum($var) = $var_summed\n";
  print "avg($var) = ".($var_summed/$wcnt)."\n";
}
my $total = $cd+$cg+$ic;
printf( "correct guess: %8i (%5.2f)\n", $cg, $cg/$total*100 );
printf( "correct dist.: %8i (%5.2f)\n", $cd, $cd/$total*100 );
printf( "correct total: %8i (%5.2f)\n", ($cd+$cg), ($cd+$cg)/$total*100 );
printf( "incorrect    : %8i (%5.2f)\n", $ic, $ic/$total*100 );
printf( "total guesses: %8i\n", $total );

# To compare: judge sentences (as in mbmt), compare "rankings" from both.
