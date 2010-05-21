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
my $fword   = $opt_f || 0; # file
my $lc      = $opt_l || 0;
my $rc      = $opt_r || 0;

#------------------------------------------------------------------------------

# In the px file:
#
my $px_target_pos = $lc + $rc + 0;
my $px_icu_pos    = $lc + $rc + 5;

# In the mg file:
#
my $mg_target_pos = $lc + $rc + 0;
my $mg_c_name_pos = $mg_target_pos + 2;
my $mg_c_type_pos = $mg_target_pos + 6;
my $mg_icu_pos    = $lc + $rc + 7;

# ----

my %scores;

open(FHM, $mg_file) || die "Can't open mg file.";
open(FHP, $px_file) || die "Can't open px file.";

while ( my $mline = <FHM> ) {

  chomp $mline;

  if ( my $pline = get_next(*FHP) ) {

      my @px_parts = split (/ /, $pline);
      my $pxtarget = $px_parts[ $px_target_pos ];
      my $px_icu   = $px_parts[ $px_icu_pos ];

      my @mg_parts  = split (/ /, $mline);
      my $target = $mg_parts[ $mg_target_pos ];
      my $c_type = $mg_parts[ $mg_c_type_pos ];
      my $c_name = $mg_parts[ $mg_c_name_pos ];
      my $mg_icu = $mg_parts[ $mg_icu_pos ];
      #print "$target\n";
      #if ( $c_type eq "G" ) { # gated!
	  #print "mg: $mline\n";
	  #print "px: $pline\n";
	  if ( $mg_icu eq $px_icu ) {
	      #print "EQUAL\n";
	  }
	  print "\n";
	  ++$scores{'mg'}{$c_name}{$mg_icu};
	  ++$scores{'px'}{$c_name}{$px_icu};
      #}

  }
}

close(FHP);
close(FHM);

my @infos = ('cg', 'cd', 'ic');
my @files = ( 'px', 'mg' );

foreach my $file ( @files ) {
    print "$file:\n";
    foreach my $c_name (sort (keys( %{$scores{$file}} ))) {
	print "$c_name: ";
	foreach my $info ( @infos ) {
	    if ( defined $scores{$file}{$c_name}{$info} ) {
		print $info.":".$scores{$file}{$c_name}{$info};
	    } else {
		print "$info:0";
	    }
	    print " ";
	}
	print "\n";
    }
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

__END__

px output:

# instance+target classification logprob entropy word_lp guess (dist.cnt [topn])
_ _ The `` -3.34807 8.24174 10.1829 cd 1 0 9103 [ `` 14787 The 10598 But 4149 In 2877 He 2869 ]
_ The Plastics Derby -21.1142 10.5035 2.26994e+06 icu 2 1 3236 [ Derby 152 New 111 company 96 first 81 two 74 ]

mg output:

eat nothing more [ dflt 0 to 0.0531758 D cd 1 0 ] 155 2031 { to 306 }
nothing more Mrs [ dflt 0 to 0 D ic 2 1 ] 15 96 { to 36 }
