#!/usr/bin/perl -w
# $Id$
#
use strict;
use Getopt::Std;

#------------------------------------------------------------------------------
# User options
#
# compare_px_srilm.pl -f nyt.l1000_20100120_1e5.px -l 2
#
#------------------------------------------------------------------------------

use vars qw/ $opt_f $opt_i $opt_l $opt_r /;

getopts('f:s:il:r:');

my $wopr_file  = $opt_f || 0;
my $ignore_oov = $opt_i || 0;
my $lc         = $opt_l || 0;
my $rc         = $opt_r || 0;

#------------------------------------------------------------------------------

my %summary;
my $log2prob_pos = $lc + $rc + 2;
my $target_pos   = $lc + $rc + 0;
my $unknown_pos  = $lc + $rc + 6;
my $md_pos       = $lc + $rc + 7; #match depth
my $ml_pos       = $lc + $rc + 8; #matched at leaf

my $wopr_sumlog10  = 0.0;
my $srilm_sumlog10 = 0.0;
my $wordcount = 0;
my $sentencecount = 0;
my $oovcount = 0;

my $f = "%02b"; # Number of binary indicators

open(FHW, $wopr_file)  || die "Can't open file.";

while ( my $line = <FHW> ) {

    if ( substr($line, 0, 1) eq "#" ) {
      next;
    }
    my @parts = [];
    my $target;
    my $wopr_log2prob;
    my $wopr_log10prob;
    my $wopr_prob;
    my $icu;
    my $md;
    my $ml;
    my $extra = 0;
    my $indicators = 0;
    
    @parts  = split (/ /, $line);
    $target         = $parts[ $target_pos ];
    $wopr_log2prob  = $parts[ $log2prob_pos ]; # native logp value
    $wopr_prob      = 2 ** $wopr_log2prob;
    $wopr_log10prob = log2tolog10( $wopr_log2prob );
    $icu            = $parts[$unknown_pos];
    $md             = $parts[$md_pos];
    $ml             = $parts[$ml_pos];

    if ( $ignore_oov ) {
      $icu = "k";
    }
    if ( $target eq "<\/s>" ) {
      ++$sentencecount;
    } else {
      ++$wordcount;
    }
    if ( $icu eq "u" ) {
      $indicators += 1;
      ++$oovcount;
    }
    if ( $ml == 1 ) {
      $indicators += 2;
    }


    if ( ($icu eq "k") && ($wopr_log10prob != 0) ) {
      $wopr_sumlog10 += $wopr_log10prob;
    }

    printf( "%.8f %8.4f %8.4f ", $wopr_prob, $wopr_log2prob, $wopr_log10prob );
    printf( "%2i %1i ", $md, $ml );

    #
    printf( "[$f] ", $indicators );
    print "$target";
    print "\n";
    ++$summary{$indicators};
  } 


my $tot = 0;
foreach my $key (sort (keys(%summary))) {
  $tot += $summary{$key};
}
foreach my $key (sort (keys(%summary))) {
  printf( "$f:%6i (%6.2f%%)\n", $key, $summary{$key},  $summary{$key}*100/$tot );
}
print   "    ------\n";
printf( "    %6i\n", $tot );

print "\n";
print "Sum log10 probs:\n";
printf( "Wopr:  %8.2f\n", $wopr_sumlog10 );
printf( "Wordcount: %i sentencecount: %i oovcount: %i\n", 
	$wordcount, $sentencecount, $oovcount );
if ( ! $ignore_oov ) {
  printf( "Wopr ppl:  %8.2f ", 10 ** (-$wopr_sumlog10/($wordcount-$oovcount+$sentencecount)));
  printf( "Wopr ppl1:  %8.2f ", 10 ** (-$wopr_sumlog10/($wordcount-$oovcount)));
  print " (No oov words.)\n";
} else  {
  printf( "Wopr ppl:  %8.2f ", 10 ** (-$wopr_sumlog10/($wordcount+$sentencecount)));
  printf( "Wopr ppl1:  %8.2f ", 10 ** (-$wopr_sumlog10/($wordcount)));
  print " (With oov words.)\n";
}

close(FHW);

# NB, perl log is base e.
#
sub log2tolog10 {
  my $l    = shift;
  my $e    = 2.718281828;
  my $log2 = 0.3010299957;

  return $l * $log2;
}

__END__

px output:

# instance+target classification logprob entropy word_lp guess (dist.cnt [topn])
_ _ The `` -3.34807 8.24174 10.1829 cd 1 0 9103 [ `` 14787 The 10598 But 4149  ]
_ The Plastics Derby -21.1142 10.5035 2.26994e+06 icu 2 1 3236 [ Derby 152 New 74 ]

