#!/usr/bin/perl -w
# $$
#
use strict;
use Getopt::Std;
use POSIX qw(log10);

#------------------------------------------------------------------------------
#durian:doc pberck$ head rmt.t1000_WOPR.ngd3
# # target l2p n d.count d.sum rr [ top3 ]
# + -8.73809 1 77513 1728063 0.0232558 [ ]
# BANCO -10.3977 2 740 4026 0.0222222 [ 44 373 31 162 + 99 ]
# COPEL 0 0 0 0 0 [ ]
#
# SRILM:
# # target l2p n d.count d.sum rr [ top3 ]
# + -8.73809 1 77513 1728063 0.0232558 [ ]
# BANCO -10.8105 2 740 4026 0.0222222 [ 44 373 31 162 + 99 ]
# COPEL 0 0 0 0 0 [ ]
#
# WOPRIG:
# # instance+target classification logprob entropy word_lp guess k/u md mal dist.cnt dist.sum RR ([topn])
# _ _ + The -8.83154 9.48661 455.573 cd k 1 0 12066 100226 0.0322581 [ The 7923 ( 7743 " 6513 ]
# _ + BANCO BANK -6.1964 6.1294 73.3333 cd k 2 1 75 220 0.333333 [ FANNIE 6 ONTARIO 6 ITALY 6 ]
#
# perl ../etc/compare_ngd.pl -w rmt.t1000_WOPR.ngd3 -s rmt.t1000_SRILM.ngd3 
# ...
# Grep on EL/UL for equal/unequal logprob
#         EN/UN                   n
#         EC/UC                   dist count
#         ES/US                   dist sum freq
#         ER/UR                   rr
#------------------------------------------------------------------------------

use vars qw/ $opt_l $opt_o $opt_p $opt_r $opt_s $opt_w $opt_L /; #oneliner, px, srilm, wopr

getopts('l:op:r:s:w:L:');

my $lc         = $opt_l || 0;
my $oneliner   = $opt_o || 0;
my $px_file    = $opt_p || 0;
my $rc         = $opt_r || 0;
my $srilm_file = $opt_s || 0;
my $wopr_file  = $opt_w || 0;
my $log_base   = $opt_L || 2; # for reading px file

#------------------------------------------------------------------------------

my $ngd_target_pos = 0;
my $ngd_logp_pos = 1;
my $ngd_n_pos = 2;
my $ngd_dcount_pos = 3;
my $ngd_dsumf_pos = 4;
my $ngd_rr_pos = 5;

my $px_target_pos   = $lc + $rc + 0;
my $px_log2prob_pos = $lc + $rc + 2;
my $px_unknown_pos  = $lc + $rc + 5;
my $px_n_pos        = $lc + $rc + 7; # match depth
my $px_dcount_pos   = $lc + $rc + 9;
my $px_dsumf_pos    = $lc + $rc + 10;
my $px_rr_pos       = $lc + $rc + 11;


# Stats per n-gram size?

my $lines = 0;
my $oov = 0;

my $equal_logp = 0;
my $equal_n = 0;
my $equal_dcount = 0;
my $equal_sumf = 0;
my $equal_rr = 0;

my $sum_ngd_logp = 0.0;
my $sum_srilm_logp = 0.0;

my %equal_logp_n;

my %nc_wopr; #n counts
my %nc_srilm;

# Per twee?
#
if ( $px_file && $wopr_file ) {
  open(FHP, $px_file)    || die "Can't open px file.";
  open(FHW, $wopr_file)  || die "Can't open wopr file.";

  # bu!
  #
  while ( (my $lw = get_next(*FHW)) && (my $lp = get_next(*FHP)) ) {
    #print "$lw\n$lp\n";
    # lists                   -4.98229 1 77513 1728063 0.00138696 [ ]
    # tops turnover lists of -16.6096 4.38902 100000 ic u 1 0 59 237 0
    #
    # tops                      -4.93653 1 77513 1728063 0.00139082 [ ]
    # Food group tops Nutricia -16.6096 0.918296 100000 ic u 2 1 2 3 0

    my @lwp = split (/ /, $lw);
    my @lpp = split (/ /, $lp);

    # From the .px file:
    #
    my $px_target         = $lpp[ $px_target_pos ];
    my $px_wopr_log2prob  = $lpp[ $px_log2prob_pos ]; # or log10, depending on parameter
    my $px_wopr_prob      = 2 ** $px_wopr_log2prob;
    my $px_wopr_log10prob = log2tolog10( $px_wopr_log2prob );
    if ( $log_base == 10 ) {
      $px_wopr_log10prob  = $lpp[ $px_log2prob_pos ]; 
      $px_wopr_prob       = 10 ** $px_wopr_log2prob;
      $px_wopr_log2prob  = log10tolog2( $px_wopr_log10prob );
    }
    #print " $px_wopr_log2prob = $px_wopr_log10prob\n";

    # From the .ngd3 file:
    #
    #my $ngd_logb = $lwp[$ngd_logp_pos];

    # Output
    unless ( $oneliner ) {
      printf( "% 8.4f % 8.4f ", $lwp[$ngd_logp_pos], $px_wopr_log10prob ); # Assume ngd3 has log10s
    }

    unless ( $oneliner ) {
      printf( "%1i %1i ", $lwp[$ngd_n_pos], $lpp[$px_n_pos] );
    }
    $nc_wopr{$lwp[$ngd_n_pos]} += 1;
    $nc_srilm{$lpp[$px_n_pos]} += 1;

    unless ( $oneliner ) {
      if (  ($lwp[$ngd_dcount_pos] > 999999) || ($lpp[$px_dcount_pos] > 999999) ) {
	printf( "%7i %7i ", $lwp[$ngd_dcount_pos], $lpp[$px_dcount_pos] );
      } else {
	printf( "%6i %6i ", $lwp[$ngd_dcount_pos], $lpp[$px_dcount_pos] );
      }
    }

    unless ( $oneliner ) {
      printf( "%7i %7i ", $lwp[$ngd_dsumf_pos], $lpp[$px_dsumf_pos] );
    }

    unless ( $oneliner ) {
      printf( "%5.3f %5.3f", $lwp[$ngd_rr_pos], $lpp[$px_rr_pos] );
    }

    # Text flags
    #
    if ( equal($lwp[$ngd_logp_pos], $px_wopr_log10prob) ) {
      unless ( $oneliner ) {
	print " EL ";
      }
      ++$equal_logp;
    } else {
      unless ( $oneliner ) {
	print " UL ";
      }
    }
    #
    if ( $lwp[$ngd_n_pos] == $lpp[$px_n_pos] ) {
      unless ( $oneliner ) {
	print "EN ";
      }
      ++$equal_n;
    } else {
      unless ( $oneliner ) {
	print "UN ";
      }
    }
    if ( $lwp[$ngd_n_pos] == 0 ) {
      ++$oov;
    } else {
      # Add probs for non OOV words
      $sum_ngd_logp += $lwp[$ngd_logp_pos];
      $sum_srilm_logp += $px_wopr_log10prob;
    }
    #
    if ( $lwp[$ngd_dcount_pos] == $lpp[$px_dcount_pos] ) {
      unless ( $oneliner ) {
	print "EC ";
      }
      ++$equal_dcount;
    } else {
      unless ( $oneliner ) {
	print "UC ";
      }
    }
    #
    if ( $lwp[$ngd_dsumf_pos] == $lpp[$px_dsumf_pos] ) {
      unless ( $oneliner ) {
	print "ES ";
      }
      ++$equal_sumf;
    } else {
      unless ( $oneliner ) {
	print "US ";
      }
    }
    if ( equal($lwp[$ngd_rr_pos], $lpp[$px_rr_pos]) ) {
      unless ( $oneliner ) {
	print "ER ";
      }
      ++$equal_rr;
    } else {
      unless ( $oneliner ) {
	print "UR ";
      }
    }
    unless ( $oneliner ) {
      print $lwp[$ngd_target_pos];
      print "\n";
    }

    ++$lines;
    
  }

  close( FHW );
  close( FHP );
}

if ( $wopr_file && $srilm_file ) {
  #
  # Compare two output files from ngt experiments. Assume one is from an SRILM LM.
  #
  open(FHW, $wopr_file)  || die "Can't open wopr file.";
  open(FHS, $srilm_file) || die "Can't open srilm file.";

  while ( (my $lw = get_next(*FHW)) && (my $ls = get_next(*FHS)) ) {
    my @lwp = split (/ /, $lw);
    my @lsp = split (/ /, $ls);

    unless ( $oneliner ) {
      printf( "% 8.4f % 8.4f ", $lwp[$ngd_logp_pos], $lsp[$ngd_logp_pos] );
    }

    unless ( $oneliner ) {
      printf( "%1i %1i ", $lwp[$ngd_n_pos], $lsp[$ngd_n_pos] );
    }
    $nc_wopr{$lwp[$ngd_n_pos]} += 1;
    $nc_srilm{$lsp[$ngd_n_pos]} += 1; #both srim/px in this variable

    unless ( $oneliner ) {
      if (  ($lwp[$ngd_dcount_pos] > 999999) || ($lsp[$ngd_dcount_pos] > 999999) ) {
	printf( "%7i %7i ", $lwp[$ngd_dcount_pos], $lsp[$ngd_dcount_pos] );
      } else {
	printf( "%6i %6i ", $lwp[$ngd_dcount_pos], $lsp[$ngd_dcount_pos] );
      }
    }

    unless ( $oneliner ) {
      printf( "%7i %7i ", $lwp[$ngd_dsumf_pos], $lsp[$ngd_dsumf_pos] );
    }

    unless ( $oneliner ) {
      printf( "%5.3f %5.3f", $lwp[$ngd_rr_pos], $lsp[$ngd_rr_pos] );
    }

    # Print text flags
    # Maybe count these too, hence oneliner inside each possibility.
    #
    if ( equal($lwp[$ngd_logp_pos], $lsp[$ngd_logp_pos]) ) {
      unless ( $oneliner ) {
	print " EL ";
      }
      ++$equal_logp;
    } else {
      unless ( $oneliner ) {
	print " UL ";
      }
    }
    #
    if ( $lwp[$ngd_n_pos] == $lsp[$ngd_n_pos] ) {
      unless ( $oneliner ) {
	print "EN ";
      }
      ++$equal_n;
    } else {
      unless ( $oneliner ) {
	print "UN ";
      }
    }
    if ( $lwp[$ngd_n_pos] == 0 ) {
      ++$oov;
    } else {
      # Add probs for non OOV words
      $sum_ngd_logp += $lwp[$ngd_logp_pos];
      $sum_srilm_logp += $lsp[$ngd_logp_pos];
    }
    #
    if ( $lwp[$ngd_dcount_pos] == $lsp[$ngd_dcount_pos] ) {
      unless ( $oneliner ) {
	print "EC ";
      }
      ++$equal_dcount;
    } else {
      unless ( $oneliner ) {
	print "UC ";
      }
    }
    #
    if ( $lwp[$ngd_dsumf_pos] == $lsp[$ngd_dsumf_pos] ) {
      unless ( $oneliner ) {
	print "ES ";
      }
      ++$equal_sumf;
    } else {
      unless ( $oneliner ) {
	print "US ";
      }
    }
    if ( equal($lwp[$ngd_rr_pos], $lsp[$ngd_rr_pos]) ) {
      unless ( $oneliner ) {
	print "ER ";
      }
      ++$equal_rr;
    } else {
      unless ( $oneliner ) {
	print "UR ";
      }
    }
    unless ( $oneliner ) {
      print $lwp[$ngd_target_pos];
      print "\n";
    }

    ++$lines;
  }

  close(FHS);
  close(FHW);
}

unless ( $oneliner ) {
  print "Lines: $lines\n";
  print "Oov: $oov\n";
  print "Equal logp: $equal_logp\n";
  print "Equal n: $equal_n\n";
  print "Equal dcount: $equal_dcount\n";
  print "Equal sumf: $equal_sumf\n";
  print "Equal rr: $equal_rr\n";

  print "Wopr:     ";
  foreach my $n (sort (keys( %nc_wopr ))) {
    print "$n:$nc_wopr{$n} ";
  }
  print "\n";

  print "Srilm/PX: ";
  foreach my $n (sort (keys( %nc_srilm ))) {
    print "$n:$nc_srilm{$n} ";
  }
  print "\n";
  
  print "Ngd   sum logp: $sum_ngd_logp\n";
  print "Srilm sum logp: $sum_srilm_logp\n";

} else {
  print "$lines $oov $equal_logp $equal_n $equal_dcount $equal_sumf $equal_rr\n";
}

sub get_next {
 my $h = shift;
 while ( my $line = <$h> ) {
   if ( substr($line, 0, 1) eq "#" ) {
     return get_next($h);
   }
   chomp $line;
   return $line;
 }
 return 0;
}

sub equal {
  my $v0 = shift;
  my $v1 = shift;
  my $ep = 0.00004;
  if ( abs(($v1-$v0)) < $ep ) {
    return 1;
  }
  return 0;
}

sub repr_equal {
  my $v0 = shift;
  my $v1 = shift;
  
  my $v0r = sprintf( "%.4f", $v0 );
  my $v1r = sprintf( "%.4f", $v1 );
  print "[$v0r,$v1r]";
  if ( $v0r eq $v1r ) {
    return 1;
  }
  return 0;
}

# NB, perl log is base e.
#
sub log2tolog10 {
  my $l    = shift;
  my $e    = 2.718281828;
  my $log2 = 0.3010299957;

  return $l * $log2;
}

sub log10tolog2 {
  my $l     = shift;
  my $e     = 2.718281828;
  my $log10 = 3.321928095;

  return $l * $log10;
}

__END__
