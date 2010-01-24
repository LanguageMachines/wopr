#!/usr/bin/perl -w
# $Id$
#
use strict;
use Getopt::Std;

#------------------------------------------------------------------------------
#
#------------------------------------------------------------------------------

use vars qw/ $opt_f $opt_s $opt_v /;

getopts('f:s:v');

my $wopr_file  = $opt_f || 0;
my $srilm_file = $opt_s || 0;
my $verbose    = $opt_v || 0;

#------------------------------------------------------------------------------

my %summary;

my $log2prob_pos  = 0;
my $wopr_pplx_pos = 1;

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

	my $wopr_ppl  = $parts[$wopr_pplx_pos]; #ngp is like SRILM, no OOV

	printf( "%7.2f ", $wopr_ppl );
	printf( "%7.2f %7.2f\n", $srilm_ppl, $srilm_ppl1 );
	
	my $indicator = 0;
	my $ratio = $wopr_ppl / $srilm_ppl;
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

ngp3 output:
-114.882 295.259 16 2 The Plastics seem fascinated with Cady's passion for school and ignorance of American popular culture .
-114.913 295.717 15 1 `` You're like a Martian , '' Regina , the Alpha Plastic , says .
-68.2872 864.258 7 0 Cady becomes an experiment for them .
