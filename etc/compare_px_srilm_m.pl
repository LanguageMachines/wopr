#!/usr/bin/perl -w
# $Id$
#
use strict;
use Getopt::Std;
use POSIX qw(log10);

#------------------------------------------------------------------------------
# For output with </s>. Calculates the SRILM ppl, ppl1.
#
# compare_px_sril2m.pl -f nyt.il1000_20100120_1e5.ngt3 
#                      -s nyt.tail1000.ngramlm_yt.1e5.srilm.dbg2.out -l 2
#
# Wopr prob  SRILM prob n   
# 0.00013040 0.00013040 1    1.0 [00] ,
# 0.00337839 0.00166020 2    2.0 [01] Yeardley
# 0.00002820 0.00000818 1    3.4 [01] Smith
#
#------------------------------------------------------------------------------

use vars qw/ $opt_f $opt_s $opt_v $opt_l $opt_r /;

getopts('f:s:vl:r:');

my $wopr_file  = $opt_f || 0;
my $srilm_file = $opt_s || 0;
my $verbose    = $opt_v || 0;
my $lc         = $opt_l || 0;
my $rc         = $opt_r || 0;

#------------------------------------------------------------------------------

my %summary;
my $log2prob_pos = $lc + $rc + 2;
my $target_pos   = $lc + $rc + 0;
my $unknown_pos  = $lc + $rc + 5;

my $wopr_sumlog10  = 0.0;
my $srilm_sumlog10 = 0.0;
my $wordcount = 0;
my $sentencecount = 0;
my $oovcount = 0;

open(FHW, $wopr_file)  || die "Can't open file.";
open(FHS, $srilm_file) || die "Can't open file.";

while ( my $line = <FHS> ) {

    if ( $line =~ /p\( / ) {
	#print $line;
	
	my $srilm_prob = 0;
	my $srilm_n    = "0";#OOV is 0
	if ( $line =~ /\[(\d)gram\] (.*) \[/ ) {
	    $srilm_n = $1;#substr($1, 0, 1);
	    $srilm_prob = $2;
	}
	#print "$srilm_prob\n";

	my $wline;
	my @parts = [];
	my $target;
	my $wopr_log2prob;
	my $wopr_prob;
	my $icu;
	my $extra = 0;
	if ( $line =~ /p\( <\/s> / ) {
	    ++$sentencecount;
	} else {
	    ++$wordcount;
	}
	if ( 1 == 2 ) { #$line =~ /p\( <\/s> / ) {
	  $extra = 1;
	  $target        = "</s>";
	  $wopr_log2prob = 0.0;
	  $wopr_prob     = 0.0;
	  $icu     = "..";
	} else {
	  $wline = get_next_wopr();
	  @parts  = split (/ /, $wline);
	  $target        = $parts[ $target_pos ];
	  $wopr_log2prob = $parts[ $log2prob_pos ];
	  $wopr_prob     = 2 ** $wopr_log2prob;
	  $icu           = $parts[$unknown_pos];
	}

	if ( $icu eq "icu" ) {
	    $wopr_prob = 0.0;
	}

	printf( "%.8f ", $wopr_prob );
	printf( "%.8f ", $srilm_prob );

	if ( $wopr_prob > 0 ) {
	    $wopr_sumlog10 += log10($wopr_prob);
	}
	if ( $srilm_prob > 0 ) {
	    $srilm_sumlog10 += log10($srilm_prob);
	}

	print "$srilm_n ";
	my $indicators = 0;
	if ( $srilm_prob > 0 ) {
	    printf( "%6.1f ", $wopr_prob / $srilm_prob );
	    if ( $wopr_prob / $srilm_prob >= 2 ) { 
		$indicators += 1;
	    }
	    if ( $wopr_prob / $srilm_prob < 0.5 ) { 
		$indicators += 2;
	    }
	} else {
	    printf( "%6.1f ", 0 );
	}
	if ( $icu eq "icu" ) {
	    $indicators += 4;
	    ++$oovcount;
	}
	#
	# __1 - wopr_Prob >= 2*srilm_Prob
	# _1_ - wopr_Prob < 1/2*srilm_Prob
	# 1__ - unknown words
	#
	printf( "[%03b] ", $indicators );
	print "$target";
	print "\n";
	++$summary{$indicators};
    } 
}

my $tot = 0;
foreach my $key (sort (keys(%summary))) {
    $tot += $summary{$key};
}
foreach my $key (sort (keys(%summary))) {
    printf( "%03b:%6i (%6.2f%%)\n", $key, $summary{$key},  $summary{$key}*100/$tot );
}
print   "    ------\n";
printf( "    %6i\n", $tot );
print "\n";
print "Sum log10 probs:\n";
printf( "Wopr:  %8.2f\n", $wopr_sumlog10 );
printf( "SRILM: %8.2f\n", $srilm_sumlog10 );
printf( "Wordcount: %i sentencecount: %i oovcount: %i\n", 
	$wordcount, $sentencecount, $oovcount );
printf( "Wopr ppl:  %8.2f ", 10 ** (-$wopr_sumlog10/($wordcount-$oovcount+$sentencecount)));
printf( "Wopr ppl1:  %8.2f\n", 10 ** (-$wopr_sumlog10/($wordcount-$oovcount)));
printf( "SRILM ppl: %8.2f ", 10 ** (-$srilm_sumlog10/($wordcount-$oovcount+$sentencecount)));
printf( "SRILM ppl1: %8.2f\n", 10 ** (-$srilm_sumlog10/($wordcount-$oovcount)));

close(FHS);
close(FHW);

sub get_next_wopr {
    my $line = <FHW>;
    if ( substr($line, 0, 1) eq "#" ) {
	return get_next_wopr();
    }
    chomp $line;
    return $line;
}

# NB, perl log is base e.
#
sub log2tolog10 {
  my $l    = shift;
  my $e    = 2.718281828;
  my $log2 = 0.3010299957;

  return $l * $log2;
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

px output:

# instance+target classification logprob entropy word_lp guess (dist.cnt [topn])
_ _ The `` -3.34807 8.24174 10.1829 cd 1 0 9103 [ `` 14787 The 10598 But 4149 In 2877 He 2869 ]
_ The Plastics Derby -21.1142 10.5035 2.26994e+06 icu 2 1 3236 [ Derby 152 New 111 company 96 first 81 two 74 ]

