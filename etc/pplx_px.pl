#!/usr/bin/perl -w
# $Id$
#
use strict;
use Getopt::Std;
#use Math::BigFloat;
#Math::BigFloat->accuracy(10);

#------------------------------------------------------------------------------
# User options
# ...
# Assumes the output is generated with log:2 (the default), else use -L10.
# The .px file needs to be generated with the lexicon, otherwise all
# words are "unknown".
#------------------------------------------------------------------------------

use vars qw/ $opt_f $opt_g $opt_i $opt_l $opt_r $opt_R $opt_t $opt_w $opt_L $opt_O $opt_T /;

getopts('f:g:il:r:R:t:wL:OT');

my $wopr_file  = $opt_f || 0;
my $gcs        = $opt_g || 0; #for global context
my $ignore_oov = $opt_i || 0;
my $lc         = $opt_l || 0;
my $rc         = $opt_r || 0;
my $gct        = $opt_t || 0; #global context type
my $rfl        = $opt_R || 0; #RFL file for gct == 1 feature count
my $wordscore  = $opt_w || 0;
my $logbase    = $opt_L || 2;
my $oneliner   = $opt_O || 0;
my $mrr_total  = $opt_T || 0;

#------------------------------------------------------------------------------

if ( $gct == 2 ) {
  $gcs = 1
}

if ( $gct == 1 ) {
    my $count = 0;
    open(FILE, "< $rfl") or die "can't open $rfl: $!";
    $count++ while <FILE>;
    close FILE;
    $gcs = $count;
}

my %summary;
my @vsum;
for ( my $i = 0; $i < 6; $i++ ) {
  $vsum[$i] = 0;
}
# instance+target classification log2prob entropy word_lp guess k/u md mal dist.cnt dist.sum RR ([topn])
# _ _ An `` -8.92516 8.24174 486.117 cd k 1 0 9103 107918 0.0196078 [ `` 14787 The 10598 But 4149 ]
#     0  1  2        3       4       5  6 7 8 9    10     11        12

my @vsum_txt = ( "unk", "match leaf", "cg", "cd", "ig" );
my %word_score;			#cg,cd,ic counts
my $log2prob_pos  = $lc + $rc + $gcs + 2;
my $target_pos    = $lc + $rc + $gcs + 0;
my $class_pos     = $lc + $rc + $gcs + 1; #classification
my $classtype_pos = $lc + $rc + $gcs + 5; #cd/cg/uic
my $unknown_pos   = $lc + $rc + $gcs + 6;
my $md_pos        = $lc + $rc + $gcs + 7;  #match depth
my $ml_pos        = $lc + $rc + $gcs + 8;  #matched at leaf
my $dcnt_pos      = $lc + $rc + $gcs + 9;  #dist count
my $dsum_pos      = $lc + $rc + $gcs + 10; #dist sum (of freqs in dist)
my $rr_pos        = $lc + $rc + $gcs + 11; #matched at leaf

my $wopr_sumlog10  = 0.0;
my $srilm_sumlog10 = 0.0;
my $wordcount = 0;
my $sentencecount = 0;
my $oovcount = 0;
my $dcnt_sum = 0;
my $dsum_sum = 0;
my %rr_sum;
my %rr_count;

my $f = "%05b";			# Number of binary indicators

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
  if ( $logbase == 2 ) {
    $wopr_log2prob  = $parts[ $log2prob_pos ]; # native logp value, log2
    $wopr_prob      = 2 ** $wopr_log2prob;
    #my $x = Math::BigFloat->new(2);
    #my $tmp = $x->bpow($wopr_log2prob);
    $wopr_log10prob = log2tolog10( $wopr_log2prob );
  } else {
    $wopr_log10prob  = $parts[ $log2prob_pos ]; #log2prob_pos is log10
    $wopr_prob      = 10 ** $wopr_log10prob;
    $wopr_log2prob = log10tolog2( $wopr_log10prob );
  }
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

  $word_score{$target}{$classtype} += 1;

  $dcnt_sum += $dcnt;
  $dsum_sum += $dsum;

  if ( ($icu eq "k") && ($wopr_log10prob != 0) ) {
    $wopr_sumlog10 += $wopr_log10prob;
  }

  if ( $wordscore == 0 ) {
    printf( "%.5f %8.4f %8.4f ", $wopr_prob, $wopr_log2prob, $wopr_log10prob );
    printf( "%2i %1i ", $md, $ml );

    #
    printf( "[$f] ", $indicators );
    print "$target";
    print "\n";
    ++$summary{$indicators};
  }
} 

close(FHW);

if ( $wordscore == 0 ) {
  my $tot = 0;
  foreach my $key (sort (keys(%summary))) {
    $tot += $summary{$key};
  }

  foreach my $key (sort { $a <=> $b } (keys(%summary))) {
    printf( "$f:%6i (%6.2f%%)\n", $key, $summary{$key},  $summary{$key}*100/$tot );
  }

  my ($o_adc, $o_ads);
  printf ("dist_freq sum: %6i, ave: (%6.2f)\n", $dcnt_sum, $dcnt_sum/$tot);
  printf ("dist_sum sum: %6i, ave: (%6.2f)\n", $dsum_sum, $dsum_sum/$tot);
  $o_adc = $dcnt_sum/$tot;
  $o_ads = $dsum_sum/$tot;

  my ($o_cg, $o_cd, $o_ic);
  for ( my $i = 0; $i < 5; $i++ ) {
    my $frmt = "%".($i+1)."i";
    if ( $tot != 0 ) {
      printf( "Column: %2i %6i (%6.2f%%) %s\n", $i, $vsum[$i],  $vsum[$i]*100/$tot, $vsum_txt[$i] );
      # To add to the oneliner.
      if ( $vsum_txt[$i] eq "cg" ) {
	$o_cg = $vsum[$i]*100/$tot;
      } elsif ( $vsum_txt[$i] eq "cd" ) {
	$o_cd = $vsum[$i]*100/$tot;
      } elsif ( $vsum_txt[$i] eq "ig" ) {
	$o_ic = $vsum[$i]*100/$tot;
      }
    }
  }
  printf( "Total: %6i\n", $tot );

  print "\n";
  print "Sum log10 probs:\n";
  printf( "Wopr:  %8.2f\n", $wopr_sumlog10 );
  printf( "Wordcount: %i sentencecount: %i oovcount: %i\n", 
	  $wordcount, $sentencecount, $oovcount );
  if ( $wordcount == $oovcount ) {
    $oovcount = 0;
  }
  my ($o_pplx, $o_pplx1);
  if ( ! $ignore_oov ) {
    printf( "Wopr ppl:  %8.2f ", 10 ** (-$wopr_sumlog10/($wordcount-$oovcount+$sentencecount)));
    printf( "Wopr ppl1:  %8.2f ", 10 ** (-$wopr_sumlog10/($wordcount-$oovcount)));
    print " (No oov words.)\n";
    $o_pplx  = 10 ** (-$wopr_sumlog10/($wordcount-$oovcount+$sentencecount));
    $o_pplx1 = 10 ** (-$wopr_sumlog10/($wordcount-$oovcount));
  } else {
    printf( "Wopr ppl:  %8.2f ", 10 ** (-$wopr_sumlog10/($wordcount+$sentencecount)));
    printf( "Wopr ppl1:  %8.2f ", 10 ** (-$wopr_sumlog10/($wordcount)));
    print " (With oov words.)\n";
    $o_pplx  = 10 ** (-$wopr_sumlog10/($wordcount+$sentencecount));
    $o_pplx1 = 10 ** (-$wopr_sumlog10/($wordcount));
  }

  print "\n";
  my ($o_mrr_cg, $o_mrr_cd, $o_mrr_gd);
  foreach my $key (sort (keys(%rr_sum))) {
    if ( $mrr_total ) {
      # MRR over all the answers, 0 for wrong answers.
      printf( "RR(%s): %8.3f, MRR: %8.3f\n", $key, $rr_sum{$key}, $rr_sum{$key}/$tot);
      # To add to the oneliner.
      if ( $key eq "cg" ) {
	$o_mrr_cg = $rr_sum{$key}/$tot;
      } elsif ( $key eq "cd" ) {
	$o_mrr_cd = $rr_sum{$key}/$tot;
      } elsif ( $key eq "gd" ) {
	$o_mrr_gd = $rr_sum{$key}/$tot;
      }
    } else {
      # This calculates MRR over the correct/in-dist scores only.
      printf( "RR(%s): %8.3f, MRR: %8.3f\n", $key, $rr_sum{$key}, $rr_sum{$key}/$rr_count{$key});
      # To add to the oneliner.
      if ( $key eq "cg" ) {
	$o_mrr_cg = $rr_sum{$key}/$rr_count{$key};
      } elsif ( $key eq "cd" ) {
	$o_mrr_cd = $rr_sum{$key}/$rr_count{$key};
      } elsif ( $key eq "gd" ) {
	$o_mrr_gd = $rr_sum{$key}/$rr_count{$key};
      }
    }
  }

if ( $oneliner ) {
  my $ctx = "l".$lc."r".$rc;
  printf( "%s %i %.2f %.2f %.2f %.2f %.2f %.3f %.3f %.3f %.2f %.2f %s\n", $wopr_file, $tot, $o_cg, $o_cd, $o_ic, $o_pplx, $o_pplx1, $o_mrr_cd, $o_mrr_cg, $o_mrr_gd, $o_adc, $o_ads, $ctx );
}

}
if ( $wordscore == 1 ) {
  foreach my $key (sort (keys(%word_score))) {
    #check keys
    unless ( defined $word_score{$key}{"cg"} ) {
      $word_score{$key}{"cg"} = 0;
    }
    unless ( defined $word_score{$key}{"cd"} ) {
      $word_score{$key}{"cd"} = 0;
    }
    unless ( defined $word_score{$key}{"ic"} ) {
      $word_score{$key}{"ic"} = 0;
    }
    my $tot = $word_score{$key}{"cg"}+$word_score{$key}{"cd"}+$word_score{$key}{"ic"};
    printf( "%s: cg:%s (%.2f%%) cd:%s (%.2f%%) ic:%s (%.2f%%)\n", $key, $word_score{$key}{"cg"}, $word_score{$key}{"cg"}/$tot*100, $word_score{$key}{"cd"}, $word_score{$key}{"cd"}/$tot*100, $word_score{$key}{"ic"}, $word_score{$key}{"ic"}/$tot*100);
  }
}

# NB, perl log is base e.
#
sub log2tolog10 {
  my $l    = shift;
  my $e    = 2.718281828;
  my $log2 = 0.3010299957;

  return $l * $log2;
}

# NB, perl log is base e.
#
sub log10tolog2 {
  my $l     = shift;
  my $e     = 2.718281828;
  my $log10 = 3.321928095;

  return $l * $log10;
}

__END__

px output:

# instance+target classification logprob entropy word_lp guess (dist.cnt [topn])
_ _ The `` -3.34807 8.24174 10.1829 cd 1 0 9103 [ `` 14787 The 10598 But 4149  ]
  _ The Plastics Derby -21.1142 10.5035 2.26994e+06 icu 2 1 3236 [ Derby 152 New 74 ]

