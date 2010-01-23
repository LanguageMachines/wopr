#!/usr/bin/perl -w
# $Id$
#
use strict;
use Getopt::Std;

#------------------------------------------------------------------------------
#
# Shows the pplx, pplx(nOOV), ppl, ppl1 from wopr and SRILM
#
# perl compare_pxs_srilm.pl -f nyt.tail1000.l2r0_20100121a_2e7.pxs 
#                  -s nyt.tail1000.ngramlm_yt.2e7.srilm.dbg2.out -l 2 
#  84.99   84.99   61.35   61.35
#  85.77   55.10  178.77  178.77
# 189.28  189.28  111.53  111.53
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
my $target_pos   = $lc + $rc + 1;
my $unknown_pos  = $lc + $rc + 5;

open(FHW, $wopr_file)  || die "Can't open file.";
open(FHS, $srilm_file) || die "Can't open file.";

my $idx = 0;

while ( my $line = <FHS> ) {

    if ( $line =~ /file / ) {
	$line = <FHS>;
	next; #end
    }

    #logprob= -24.6601 ppl= 292.419 ppl1= 549.549
    if ( $line =~ /ppl= (.*) ppl1= (.*)/ ) {
	#print $line;

	my $srilm_ppl  = $1;
	my $srilm_ppl1 = $2;

	my $wline = get_next_wopr();
	my @parts  = split (/ /, $wline);

	my $sum_l2p        += $parts[2];
	my $text_wordcount += $parts[1];
	my $sum_noov_l2p   += $parts[6];
	my $text_noov_wordcount += $parts[5];

	#printf( "%4d ", $idx++ );
	my $wopr_ppl  = 2**(-$parts[2]/$parts[1]);
	my $wopr_ppl1 = 2**(-$parts[6]/$parts[5]); #comparable to SRILM

	printf( "%7.2f %7.2f ", $wopr_ppl, $wopr_ppl1 );
	printf( "%7.2f %7.2f\n", $srilm_ppl, $srilm_ppl1 );
	
	my $indicator = 0;
	my $ratio = $wopr_ppl1 / $srilm_ppl;
	if ( $ratio >= 2 ) {
	    $indicator += 1;
	} elsif ( $ratio < 0.5 ) {
	    $indicator += 2;
	}
	++$summary{$indicator};
    } 
}

my $tot = 0;
foreach my $key (sort (keys(%summary))) {
    $tot += $summary{$key};
}
foreach my $key (sort (keys(%summary))) {
    printf( "# %02b:%6i (%6.2f%%)\n", $key, $summary{$key},
	                            $summary{$key}*100/$tot );
}
print   "#    ------\n";
printf( "#    %6i\n", $tot );

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

pxs output:
# nr. #words sum(log2prob) avg.pplx avg.wordlp #nOOV sum(nOOVl2p) std.dev(wordlp) [wordlp(each word)]
0 16 -156.428 877.178 317962 14 -114.199 807377 [ 10.1829 2.26994e+06 7642.88 162138 14 2.26994e+06 42829 2.94444 8918 49.5457 324276 7 1560.09 60 3 3.8 ]
1 15 -139.682 635.677 591539 14 -118.568 629021 [ 7.29817 712.417 771.038 48.1919 2.26994e+06 18.2207 11.7504 378322 6 21.5395 567484 567484 18.2207 814.007 51 ]
2 7 -73.9057 1507.52 1.60474e+06 7 -73.9057 857812 [ 2.26994e+06 18916.1 60 70935.5 127.267 143.839 5.28926 ]
