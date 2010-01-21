#!/usr/bin/perl -w
#
use strict;
use Getopt::Std;

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
  ++$file_linecount;

}

printf( "sum log2p      : %9.2f\n", $sum_l2p );
printf( "word count     : %6i\n", $text_wordcount );
printf( "ave log2p      : %9.2f\n", $sum_l2p/$text_wordcount );
printf( "ave pplxs      : %9.2f\n", 2**(-$sum_l2p/$text_wordcount) );
print "\n";
#
printf( "sum noov log2p : %9.2f\n", $sum_noov_l2p );
printf( "number noov    : %6i\n", $text_noov_wordcount );
printf( "ave noov log2p : %9.2f\n", $sum_noov_l2p/$text_noov_wordcount );
printf( "ave pplxs      : %9.2f\n", 2**(-$sum_noov_l2p/$text_noov_wordcount) );
print "\n";
printf( "number oov     : %5i\n", $text_wordcount-$text_noov_wordcount );

__END__

