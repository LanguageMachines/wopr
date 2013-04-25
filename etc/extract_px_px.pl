#!/usr/bin/perl -w
# $Id: compare_px_srilm.pl 3608 2010-01-29 10:45:12Z pberck $
#
use strict;
use Getopt::Std;

#------------------------------------------------------------------------------
# User options
#
# Like extract_px_mg, but for two px files.
#
#------------------------------------------------------------------------------

use vars qw/ $opt_c $opt_d $opt_m $opt_p $opt_f $opt_l $opt_r /;

getopts('c:df:m:p:l:r:');

my $mg_file  = $opt_m || 0;
my $px_file  = $opt_p || 0;
my $fword    = $opt_f || ""; # focus word
my $lc       = $opt_l || 0;
my $rc       = $opt_r || 0;
my $confine  = $opt_c || "";

#------------------------------------------------------------------------------

# In the px file:
#
my $px_target_pos = $lc + $rc + 0;
my $px_icu_pos    = $lc + $rc + 5;
my $px_ku_pos     = $lc + $rc + 6;
my $px_prob_pos   = $lc + $rc + 2;

# ----

open(FHM, $mg_file) || die "Can't open px file."; #is px2
open(FHP, $px_file) || die "Can't open px file."; #is px1

my $mg_sumlog2 = 0;
my $px_sumlog2 = 0;
my $wordcount = 0;
my $mg_0count = 0;
my $px_0count = 0;

while ( my $mline = get_next(*FHM) ) {

  chomp $mline;

  if ( my $pline = get_next(*FHP) ) {

      my @px_parts    = split (/ /, $pline);
      my $pxtarget    = $px_parts[ $px_target_pos ];
      my $px_icu      = $px_parts[ $px_icu_pos ];
      my $px_ku       = $px_parts[ $px_ku_pos ];
      my $px_log2prob = $px_parts[ $px_prob_pos ];

      my @px2_parts    = split (/ /, $mline);
      my $px2target    = $px2_parts[ $px_target_pos ];
      my $px2_icu      = $px2_parts[ $px_icu_pos ];
      my $px2_ku       = $px2_parts[ $px_ku_pos ];
      my $px2_log2prob = $px2_parts[ $px_prob_pos ];
      my $c_name       = "?"; #$mg_parts[ $mg_c_name_pos ];

      #             -p file -m file
      my $icu_str = $px_icu.$px2_icu;
      if ( ($c_name eq $fword) || ($fword eq "") ) {
	if ( ($confine eq "") || ($confine eq $icu_str) ) {
	  print "$pline\n$mline\n\n";
	}
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

