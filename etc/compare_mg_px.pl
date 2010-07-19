#!/usr/bin/perl -w
# $Id: compare_px_srilm.pl 3608 2010-01-29 10:45:12Z pberck $
#
use strict;
use Getopt::Std;

#------------------------------------------------------------------------------
# User options
#
# Compare the word expert output from mg to the normal
# output from pplxs.
# Reads an mg output file and a px output file.
# Extra option -b to create a list with the good gated classifiers.
# With -b one needs to specify "ic", "cg" or "cd".
#
# this.pl -m mg-file -p px-file -f focusword/classifiername
#
#------------------------------------------------------------------------------

use vars qw/ $opt_b $opt_d $opt_m $opt_p $opt_f $opt_l $opt_r $opt_P /;

getopts('b:df:m:p:l:r:P');

my $mg_file  = $opt_m || 0;
my $px_file  = $opt_p || 0;
my $fword    = $opt_f || 0; # file
my $lc       = $opt_l || 0;
my $rc       = $opt_r || 0;
my $bestlist = $opt_b || 0;
my $deltas   = $opt_d || 0;
my $percent  = $opt_P || 0;

#------------------------------------------------------------------------------

# In the px file:
#
my $px_target_pos = $lc + $rc + 0;
my $px_icu_pos    = $lc + $rc + 5;
my $px_ku_pos     = $lc + $rc + 6;
my $px_prob_pos   = $lc + $rc + 2;
my $px_mrr_pos    = $lc + $rc + 11; #MRR in px...

# In the mg file:
#
my $mg_target_pos = $lc + $rc + 0;
my $mg_c_name_pos = $mg_target_pos + 4;
my $mg_c_type_pos = $mg_target_pos + 3;
my $mg_icu_pos    = $lc + $rc + 5;
my $mg_prob_pos   = $lc + $rc + 2;
my $mg_mrr_pos    = $lc + $rc + 11;

# ----

my %scores;
my %totals;
my %mrrs;

open(FHM, $mg_file) || die "Can't open mg file.";
open(FHP, $px_file) || die "Can't open px file.";

my $mg_sumlog2 = 0;
my $px_sumlog2 = 0;
my $wordcount = 0;
my $mg_0count = 0;
my $px_0count = 0;

while ( my $mline = <FHM> ) {

  chomp $mline;

  if ( my $pline = get_next(*FHP) ) {

      my @px_parts    = split (/ /, $pline);
      my $pxtarget    = $px_parts[ $px_target_pos ];
      my $px_icu      = $px_parts[ $px_icu_pos ];
      my $px_ku       = $px_parts[ $px_ku_pos ];
      my $px_log2prob = $px_parts[ $px_prob_pos ];
      my $px_mrr      = $px_parts[ $px_mrr_pos ];

      if ( $px_mrr  eq "[" ) {
	$px_mrr = 0;
      }

      my @mg_parts  = split (/ /, $mline);
      my $target      = $mg_parts[ $mg_target_pos ];
      my $c_type      = $mg_parts[ $mg_c_type_pos ];
      my $c_name      = $mg_parts[ $mg_c_name_pos ];
      my $mg_icu      = $mg_parts[ $mg_icu_pos ];
      my $mg_log2prob = $mg_parts[ $mg_prob_pos ];
      my $mg_mrr      = $mg_parts[ $mg_mrr_pos ];

      #print "$px_log2prob $mg_prob\n";

      #if ( $px_log2prob != 0 ) {
      if ( $px_ku ne "u" ) {
	$px_sumlog2 += $px_log2prob;
      } else {
	++$px_0count;
      }

      #if ( $mg_prob != 0 ) {
      if ( $px_ku ne "u" ) {
	$mg_sumlog2 += $mg_log2prob;
      } else {
	++$mg_0count;
      }
      
      #      ++{"on"}{'mg'}{'cd'}
      ++$scores{$c_name}{'mg'}{$mg_icu};
      ++$scores{$c_name}{'px'}{$px_icu};

      $mrrs{$c_name}{'mg'}{$mg_icu} += $mg_mrr;
      $mrrs{$c_name}{'px'}{$px_icu} += $px_mrr;

      ++$totals{'mg'}{$mg_icu};
      ++$totals{'px'}{$px_icu};

      ++$wordcount;
  }
}

close(FHP);
close(FHM);

my @infos = ('cg', 'cd', 'ic');
my @files = ( 'px', 'mg' );

foreach my $c_name (sort (keys( %scores ))) {
  foreach my $file ( @files ) {	
    foreach my $info ( @infos ) {
      if ( ! (defined $scores{$c_name}{$file}{$info}) ) {
	$scores{$c_name}{$file}{$info} = 0;
      }
      if ( ! (defined $mrrs{$c_name}{$file}{$info}) ) {
	$mrrs{$c_name}{$file}{$info} = 0;
      }
    }
  }
}

my $hoera = 0;
foreach my $c_name (sort (keys( %scores ))) {

    if ( ! $bestlist ) { # "Best" is defined as least incorrect
      if ( $scores{$c_name}{'mg'}{'ic'} < $scores{$c_name}{'px'}{'ic'} ) {
	print "[mg] ";
	++$hoera;
      }
      if ( $scores{$c_name}{'mg'}{'ic'} > $scores{$c_name}{'px'}{'ic'} ) {
	print "[px] ";
      }
      if ( $scores{$c_name}{'mg'}{'ic'} == $scores{$c_name}{'px'}{'ic'} ) {
	print "[mx] ";
	#++$hoera;
      }
    } else { # print this when bestlist is true

      my $mrr_mg   = 0;
      my $numer = $mrrs{$c_name}{'mg'}{'cd'} + $mrrs{$c_name}{'mg'}{'cg'};
      my $denom = $scores{$c_name}{'mg'}{'cd'}+$scores{$c_name}{'mg'}{'cg'};
      if ( $denom != 0 ) {
	$mrr_mg = $numer / $denom;
      }
      my $mrr_px   = 0;
      $numer = $mrrs{$c_name}{'px'}{'cd'} + $mrrs{$c_name}{'px'}{'cg'};
      $denom = $scores{$c_name}{'px'}{'cd'}+$scores{$c_name}{'px'}{'cg'};
      if ( $denom != 0 ) {
	$mrr_px = $numer / $denom;
      }

      if ( $bestlist eq "ic" ) { #incorrect we want less
	if ( $scores{$c_name}{'mg'}{$bestlist} < $scores{$c_name}{'px'}{$bestlist} ) {
	  print "$c_name\n"; 
	}
      } elsif ( $bestlist eq "mrr" ) {
	if ( $mrr_mg > $mrr_px ) {
	  print "$c_name\n"; 
	}
      } else { #for the others we want more
	if ( (defined $scores{$c_name}{'mg'}{$bestlist}) &&
	     (defined $scores{$c_name}{'px'}{$bestlist})) {
	  if ( $scores{$c_name}{'mg'}{$bestlist} > $scores{$c_name}{'px'}{$bestlist} ) {
	    print "$c_name\n"; 
	  }
	}
      }
    }
    
    if ( ! $bestlist ) {
      print "$c_name: ";

      foreach my $file ( @files ) {	
	print "$file: ";
	foreach my $info ( @infos ) {
	  print $info.":".$scores{$c_name}{$file}{$info};
	  print " ";
	}
	#print "\n";
	my $mrr   = 0;
	my $numer = $mrrs{$c_name}{$file}{'cd'} + $mrrs{$c_name}{$file}{'cg'};
	my $denom = $scores{$c_name}{$file}{'cd'}+$scores{$c_name}{$file}{'cg'};
	if ( $denom != 0 ) {
	  $mrr = sprintf("%3.1f", $numer / $denom);
	}
	print "mrr:",$mrr," ";
      }

      if ( $deltas != 0 ) {
	print "d: ";
	foreach my $info ( @infos ) {
	  my $delta = $scores{$c_name}{'mg'}{$info} - $scores{$c_name}{'px'}{$info};
	  my $delta_p = 0;
	  if ( $scores{$c_name}{'px'}{$info} != 0 ) {
	    $delta_p = $delta / $scores{$c_name}{'px'}{$info} * 100.0;
	  }
	  if ( $percent == 1 ) {
	    $delta_p = sprintf("%03.1f", $delta_p);
	    print "$info:",$delta_p,"%";
	  } else {
	    print "$info:",$delta;
	  }
	  print " ";
	}
      }

      print "\n";
    }
  }

if ( ! $bestlist ) {
  print "\nTOTALS: ";
  foreach my $file ( @files ) {	
    print "$file: ";
    foreach my $info ( @infos ) {
      if ( defined $totals{$file}{$info} ) {
	print $info.":".$totals{$file}{$info};
      } else {
	print "$info:0";
      }
      print " ";
    }
    #print "\n";
  }
  print "\n";
}

if ( ! $bestlist ) {
  printf("mg ppl:  %8.2f (%i)\n", 2 ** (-$mg_sumlog2/($wordcount)), $mg_0count);
  printf("px ppl:  %8.2f (%i)\n", 2 ** (-$px_sumlog2/($wordcount)), $px_0count);
}

# ----

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

sub log2 {
  my $l    = shift;

  if ( $l == 0 ) {
    return 0;
  }
  return log($l)/log(2);
}

sub log10 {
  my $l    = shift;

  if ( $l == 0 ) {
    return 0;
  }
  return log($l)/log(10);
}

__END__

px output:

# instance+target classification logprob entropy word_lp guess (dist.cnt [topn])
_ _ The `` -3.34807 8.24174 10.1829 cd 1 0 9103 [ `` 14787 The 10598 But 4149 In 2877 He 2869 ]
_ The Plastics Derby -21.1142 10.5035 2.26994e+06 icu 2 1 3236 [ Derby 152 New 111 company 96 first 81 two 74 ]

mg output:
_ _ _ THE and -15.1839 D dflt cd k 1 1 3670 37223 [ and 2713 to 1352 of 1346 I 1319 the 889 ]
_ _ THE END END -1 D dflt cg k 1 0 2 2 [ END 1 AUTHORESS 1 ]
