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
#
# this.pl -m mg-file -p px-file -f focusword/classifiername
#
#------------------------------------------------------------------------------

use vars qw/ $opt_m $opt_p $opt_f $opt_l $opt_r/;

getopts('f:m:p:l:r:');

my $mg_file = $opt_m || 0;
my $px_file = $opt_p || 0;
my $fword   = $opt_f || 0;
my $lc      = $opt_l || 0;
my $rc      = $opt_r || 0;

#------------------------------------------------------------------------------

# In the px file:
#
my $px_target_pos = $lc + $rc + 0;

# In the mg file:
#
my $mg_target_pos = $lc + $rc + 0;
my $mg_c_name_pos = $mg_target_pos + 2;
my $mg_c_type_pos = $mg_target_pos + 6;

# ----

open(FHM, $mg_file) || die "Can't open mg file.";
open(FHP, $px_file) || die "Can't open px file.";

while ( my $mline = <FHM> ) {

  chomp $mline;

  while ( my $pline = get_next(*FHP) ) {
    my @parts  = split (/ /, $pline);
    my $target        = $parts[ $target_pos ];
  }

}

close(FHP);
close(FHM);

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

__END__

px output:

# instance+target classification logprob entropy word_lp guess (dist.cnt [topn])
_ _ The `` -3.34807 8.24174 10.1829 cd 1 0 9103 [ `` 14787 The 10598 But 4149 In 2877 He 2869 ]
_ The Plastics Derby -21.1142 10.5035 2.26994e+06 icu 2 1 3236 [ Derby 152 New 111 company 96 first 81 two 74 ]

mg output:

eat nothing more [ dflt 0 to 0.0531758 D cd 1 0 ] 155 2031 { to 306 }
nothing more Mrs [ dflt 0 to 0 D ic 2 1 ] 15 96 { to 36 }
