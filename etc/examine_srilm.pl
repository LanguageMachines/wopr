#!/usr/bin/perl -w
#
use strict;
use Getopt::Std;

#------------------------------------------------------------------------------
# User options
#
# examine_srilm.pl -f testset.txt.ws3.hpx5.ib_1e6.srilm
#
#------------------------------------------------------------------------------

use vars qw/ $opt_b $opt_e $opt_f $opt_s $opt_v $opt_w /;

getopts('b:e:f:vw:');

my $basename   = $opt_b || "out";
my $file       = $opt_f || 0;
my $verbose    = $opt_v || 0;

#------------------------------------------------------------------------------

my $wcnt =  0;
my $scnt = -1;
my $sentence_entropy = 0;
my $word_entropy = 0;

my $sum_prob = 0.0;
my $sum_l2p  = 0.0;
my $H        = 0.0;

open(FH, $file) || die "Can't open file.";

while ( my $line = <FH> ) {
  #print $line;
  if ( substr($line, 0, 1) eq "#" ) {
    next;
  }

  chomp $line;

  if ( $line =~ /(\d+) sentences/ ) {
    print "$line\n";
    $line = <FH>;
    print "$line";
    #$line = <FH>;
    print "sum_prob=$sum_prob\n";
    print "sum_l2p =$sum_l2p\n";
    print "H       =$H\n";
    print "pplx    =". (2**(-$H)) ."\n";
    #print "pplx    =". (10**(-$H)) ."\n";
    $sum_prob = 0.0;
    $sum_l2p  = 0.0;
    $H        = 0.0;
  } elsif ( $line =~ /p\( (.*) \| / ) {
    print "$1\t";
    my @parts  = split (/ /, $line);
    my $n = $#parts;
    my $p = $parts[$n-3];
    my $l10p = $parts[$n-1];
    printf( "%.8f\t", $p );
    printf( "[ %.8f ]\n", $l10p );
    $sum_prob += $p;
    $sum_l2p += log2($p);
    $H += ( $p * log2($p) ); # p x log2(p)
  } else {
    print "$line\n";
  }

}

sub log2 {
  my $x = shift;
  if ( $x == 0 ) {
    return -99;
  }
  return log($x)/log(2);
}

__END__

The Carlyle Group figures prominently in Unger's book .
        p( The | <s> )  = [2gram] 0.107482 [ -0.968665 ]
        p( Carlyle | The ...)   = [2gram] 3.42943e-06 [ -5.46478 ]
        p( Group | Carlyle ...)         = [2gram] 0.533333 [ -0.273001 ]
        p( figures | Group ...)         = [1gram] 1.86201e-06 [ -5.73002 ]
        p( prominently | figures ...)   = [2gram] 0.00370763 [ -2.4309 ]
        p( in | prominently ...)        = [3gram] 0.875 [ -0.057992 ]
        p( Unger's | in ...)    = [1gram] 1.09272e-08 [ -7.96149 ]
        p( book | Unger's ...)  = [2gram] 0.327236 [ -0.485139 ]
        p( . | book ...)        = [2gram] 0.0572967 [ -1.24187 ]
        p( </s> | . ...)        = [3gram] 0.899083 [ -0.0462004 ]
1 sentences, 9 words, 0 OOVs
0 zeroprobs, logprob= -24.6601 ppl= 292.419 ppl1= 549.549

10^-(-24.6) = 292. ...


http://www.speech.sri.com/projects/srilm/mail-archive/srilm-user/2007-February/4.html
The source code for perplexity computation is in lm/src/TextStats.cc .
However, there is no need to modify the code.
When you have different token counts (words versus morphs) the
perplexities are no longer comparable, but the log probabilities are.
You can get the log probability from the perplexity output, e.g.:

file ../ngram-count-gt/eval97.text: 5290 sentences, 38238 words, 681 OOVs
0 zeroprobs, logprob= -86334.6 ppl= 103.502 ppl1= 198.958
                                   ^^^^^^^^
Assume the "words" in this example are actually morphs, and the actual
number
of words (including sentence boundaries) is less, say, 25000.  then the
word-perplexity is

    10^ -(-86334.6 / 25000 ) = 2840.43

--Andreas
