#!/usr/bin/perl -w
# $Id$
#
use strict;
use Getopt::Std;

#------------------------------------------------------------------------------
#
# Shows a list with the perplexity of each sentence.

# examine_pxs.pl -f nyt.tail1000.l2r0_20100121_1e5.pxs 
#  285.45
#  354.38
# 1507.52
#  ...
# sum log2p      : -203123.32
# word count     :   22829
# ave log2p      :     -8.90
# ave pplxs      :    476.92
#
# sum noov log2p : -179032.00
# number noov    :   21688
# ave noov log2p :     -8.25
# ave pplxs      :    305.47
#
# number oov     :    1141
#
#------------------------------------------------------------------------------

use vars qw/ $opt_b $opt_e $opt_f $opt_g $opt_l $opt_r $opt_v $opt_w /;

getopts('b:e:f:g:l:r:v:w:');

my $file       = $opt_f || 0;

#------------------------------------------------------------------------------

my $sum_l2p = 0;
my $text_wordcount = 0;
my $sum_noov_l2p = 0;
my $text_noov_wordcount = 0;
my $file_linecount = 0;

open(FH, $file) || die "Can't open file.";

while ( my $line = <FH> ) {
  #print $line;
  if ( substr($line, 0, 1) eq "#" ) {
    next;
  }

  chomp $line;

  my @parts  = split (/ /, $line);
  if ( $#parts < 4 ) {
    next;
  }

  # nr. #words sum(logprob) avg.pplx avg.wordlp nOOV sumlp(nOOV) std.dev(wordlp) [wordlp(each word)]
  # 0   1      2            3        4          5    6           
  $sum_l2p += $parts[2];
  $text_wordcount += $parts[1];
  $sum_noov_l2p += $parts[6];
  $text_noov_wordcount += $parts[5];

  #printf( "%7.2f %7.2f\n", $parts[2]/$parts[1], 2**(-$parts[2]/$parts[1]) );
  #printf( "%7.2f %7.2f\n", $parts[6]/$parts[5], 2**(-$parts[6]/$parts[5]) );
  printf( "%7.2f ", 2**(-$parts[2]/$parts[1]) );
  printf( "%7.2f\n", 2**(-$parts[6]/$parts[5]) );

  ++$file_linecount;
}

printf( "sum log2p      : %9.2f\n", $sum_l2p );
printf( "word count     : %7i\n", $text_wordcount );
printf( "ave log2p      : %9.2f\n", $sum_l2p/$text_wordcount );
printf( "ave pplxs      : %9.2f\n", 2**(-$sum_l2p/$text_wordcount) );
print "\n";
#
printf( "sum noov log2p : %9.2f\n", $sum_noov_l2p );
printf( "number noov    : %7i\n", $text_noov_wordcount );
printf( "ave noov log2p : %9.2f\n", $sum_noov_l2p/$text_noov_wordcount );
printf( "ave pplxs      : %9.2f\n", 2**(-$sum_noov_l2p/$text_noov_wordcount) );
print "\n";
printf( "number oov     : %7i\n", $text_wordcount-$text_noov_wordcount );

__END__

