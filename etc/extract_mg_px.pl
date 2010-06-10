#!/usr/bin/perl -w
# $Id: compare_px_srilm.pl 3608 2010-01-29 10:45:12Z pberck $
#
use strict;
use Getopt::Std;

#------------------------------------------------------------------------------
# User options
#
# Extract one classifier from both output files
#
#------------------------------------------------------------------------------

use vars qw/ $opt_b $opt_d $opt_m $opt_p $opt_f $opt_l $opt_r /;

getopts('b:df:m:p:l:r:');

my $mg_file  = $opt_m || 0;
my $px_file  = $opt_p || 0;
my $fword    = $opt_f || 0; # focus word
my $lc       = $opt_l || 0;
my $rc       = $opt_r || 0;

#------------------------------------------------------------------------------

# In the px file:
#
my $px_target_pos = $lc + $rc + 0;
my $px_icu_pos    = $lc + $rc + 5;
my $px_ku_pos     = $lc + $rc + 6;
my $px_prob_pos   = $lc + $rc + 2;

# In the mg file:
#
my $mg_target_pos = $lc + $rc + 0;
my $mg_c_name_pos = $mg_target_pos + 4;
my $mg_c_type_pos = $mg_target_pos + 3;
my $mg_icu_pos    = $lc + $rc + 5;
my $mg_prob_pos   = $lc + $rc + 2;

# ----

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

      my @mg_parts  = split (/ /, $mline);
      my $target  = $mg_parts[ $mg_target_pos ];
      my $c_type  = $mg_parts[ $mg_c_type_pos ];
      my $c_name  = $mg_parts[ $mg_c_name_pos ];
      my $mg_icu  = $mg_parts[ $mg_icu_pos ];
      my $mg_prob = $mg_parts[ $mg_prob_pos ];

      if ( $c_name eq $fword ) {
	print "$mline\n$pline\n\n";
      }
  }
}

close(FHP);
close(FHM);

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

eat nothing more [ dflt 0 to 0.0531758 D cd 1 0 ] 155 2031 { to 306 }
nothing more Mrs [ dflt 0 to 0 D ic 2 1 ] 15 96 { to 36 }
