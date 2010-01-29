#!/usr/bin/perl -w
# $Id$
#
use strict;
use Getopt::Std;

#------------------------------------------------------------------------------
# User options
#
# compare_ngt_srilm.pl -f nyt.il1000_20100120_1e5.ngt3 
#                      -s nyt.tail1000.ngramlm_yt.1e5.srilm.dbg2.out 
#
# Wopr prob    SRILM prob      
# 0.02734380 2 0.02352810 2    1.2 [0000] is
# 0.00032229 2 0.00015998 2    2.0 [0010] indeed
# 0.00002335 1 0.00000903 1    2.6 [0010] alive
#
#------------------------------------------------------------------------------

use vars qw/ $opt_f $opt_s $opt_v /;

getopts('f:s:v');

my $wopr_file  = $opt_f || 0;
my $srilm_file = $opt_s || 0;
my $verbose    = $opt_v || 0;

#------------------------------------------------------------------------------

my %summary;

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
	if ( $line =~ /p\( <\/s> / ) {
	  $wline = "N.A 0 0 N.A.";
	} else {
	  $wline = get_next_wopr();
	}
	my @parts  = split (/ /, $wline);

	printf( "%.8f ", $parts[1] );
	print "$parts[2] ";
	printf( "%.8f ", $srilm_prob );
	print "$srilm_n ";
	my $indicators = 0;
	if ( $srilm_prob > 0 ) {
	    printf( "%6.1f ", $parts[1] / $srilm_prob );
	    if ( $parts[1] / $srilm_prob >= 2 ) { 
		$indicators += 2;
	    }
	    if ( $parts[1] / $srilm_prob < 0.5 ) { 
		$indicators += 4;
	    }
	} else {
	    printf( "%6.1f ", 0 );
	}
	if ( $parts[0] eq "<unk>" ) {
	    $indicators += 8;
	}
	if ( $parts[2] != $srilm_n ) {
	    $indicators += 1;
	}
	#
	# ___1 - different length grams
	# __1_ - wopr_Prob > 2*srilm_Prob
	# _1__ - wopr_Prob < 1/2*srilm_Prob
	# 1___ - <unk>
	#
	printf( "[%04b] ", $indicators );
	print "$parts[0]";
	print "\n";
	++$summary{$indicators};
    } 
}

my $tot = 0;
foreach my $key (sort (keys(%summary))) {
    $tot += $summary{$key};
}
foreach my $key (sort (keys(%summary))) {
  printf( "%04b:%6i (%6.2f%%)\n", $key, $summary{$key},  $summary{$key}*100/$tot );
}
print   "     ------\n";
printf( "     %6i\n", $tot );
  
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

ngt3 output:

name 0.00159847 2 her name
`` 0.00184162 2 name ``
<unk> 0 0 OOV
'' 0.0110364 1 ''
) 0.00662622 2 '' )
and 0.126506 3 '' ) and
the 0.0547945 3 ) and the
<unk> 0 0 OOV
