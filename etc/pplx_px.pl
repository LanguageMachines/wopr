#!/usr/bin/perl -w
# $Id$
#
use strict;
use Getopt::Std;

#------------------------------------------------------------------------------
# User options
# ...
#------------------------------------------------------------------------------

use vars qw/ $opt_f $opt_i $opt_l $opt_r /;

getopts('f:s:il:r:');

my $wopr_file  = $opt_f || 0;
my $ignore_oov = $opt_i || 0;
my $lc         = $opt_l || 0;
my $rc         = $opt_r || 0;

#------------------------------------------------------------------------------

my %summary;
my @vsum;
for ( my $i = 0; $i < 6; $i++ ) {
  $vsum[$i] = 0;
}
my @vsum_txt = ( "unk", "match leaf", "cg", "cd", "ig" );
my $log2prob_pos  = $lc + $rc + 2;
my $target_pos    = $lc + $rc + 0;
my $classtype_pos = $lc + $rc + 5;
my $unknown_pos   = $lc + $rc + 6;
my $md_pos        = $lc + $rc + 7; #match depth
my $ml_pos        = $lc + $rc + 8; #matched at leaf
my $dcnt_pos      = $lc + $rc + 9; #dist count
my $dsum_pos      = $lc + $rc + 10; #dist sum (of freqs in dist)
my $rr_pos        = $lc + $rc + 11; #matched at leaf

my $wopr_sumlog10  = 0.0;
my $srilm_sumlog10 = 0.0;
my $wordcount = 0;
my $sentencecount = 0;
my $oovcount = 0;
my $dcnt_sum = 0;
my $dsum_sum = 0;
my %rr_sum;
my %rr_count;

my $f = "%05b"; # Number of binary indicators

open(FHW, $wopr_file)  || die "Can't open file.";

while ( my $line = <FHW> ) {

  if ( substr($line, 0, 1) eq "#" ) {
    next;
  }
    chomp $line;

    my @parts = [];
    my $target;
    my $wopr_log2prob;
    my $wopr_log10prob;
    my $wopr_prob;
    my $classtype;
    my $icu;
    my $dcnt;
    my $dsum;
    my $md;
    my $ml;
    my $extra = 0;
    my $indicators = 0;
    my $rr;
    
    @parts  = split (/ /, $line);
    $target         = $parts[ $target_pos ];
    $wopr_log2prob  = $parts[ $log2prob_pos ]; # native logp value
    $wopr_prob      = 2 ** $wopr_log2prob;
    $wopr_log10prob = log2tolog10( $wopr_log2prob );
    $icu            = $parts[$unknown_pos];
    $md             = $parts[$md_pos];
    $ml             = $parts[$ml_pos];
    $classtype      = $parts[$classtype_pos];
    $rr             = $parts[$rr_pos];
    $dcnt           = $parts[$dcnt_pos];
    $dsum           = $parts[$dsum_pos];

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
      $vsum[0]++;
    }
    if ( $ml == 1 ) {
      $indicators += 2;
      $vsum[1]++;
    }
    if ( $classtype eq "cg" ) {
      $indicators += 4;
      $vsum[2]++;
    } elsif ( $classtype eq "cd" ) {
      $indicators += 8;
      $vsum[3]++;
    } elsif ( $classtype eq "ic" ) {
      $indicators += 16;
      $vsum[4]++;
    }
    if ( ! ($classtype eq "ic") ) {    
	$rr_sum{$classtype} += $rr;
	++$rr_count{$classtype};
	$rr_sum{"gd"} += $rr;
	++$rr_count{"gd"};	
    }

    $dcnt_sum += $dcnt;
    $dsum_sum += $dsum;

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

printf ("dist_freq sum: %6i ave: %6.2f\n", $dcnt_sum, $dcnt_sum/$tot);
printf ("dist_sum sum: %6i ave: %6.2f\n", $dsum_sum, $dsum_sum/$tot);

foreach my $key (sort { $a <=> $b } (keys(%summary))) {
  printf( "$f:%6i (%6.2f%%)\n", $key, $summary{$key},  $summary{$key}*100/$tot );
}
for ( my $i = 0; $i < 5; $i++ ) {
  my $frmt = "%".($i+1)."i";
  if ( $tot != 0 ) {
    printf( "Column: %2i %6i (%6.2f%%) %s\n", $i, $vsum[$i],  $vsum[$i]*100/$tot, $vsum_txt[$i] );
  }
}
printf( "Total: %6i\n", $tot );

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

print "\n";
foreach my $key (sort (keys(%rr_sum))) {
    printf( "RR(%s): %8.3f, MRR: %8.3f\n", $key, $rr_sum{$key}, $rr_sum{$key}/$rr_count{$key});
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

