#!/usr/bin/perl -w
#
use strict;
use Getopt::Std;
use utf8;
use Encode;

#binmode( STDOUT, ':utf8' );
#binmode STDOUT, 'encoding(utf8)';
binmode(STDOUT, ":encoding(UTF-8)");

#------------------------------------------------------------------------------
# User options
#
# take nrs 2 in the distro's, see which are the number 1s. Are
# they 50/50 or so confusable?
# -c creates a space-seperated list which can be read
# into run_exps.bash.
#
#------------------------------------------------------------------------------

use vars qw/ $opt_c $opt_f $opt_l $opt_r $opt_v  /;

getopts('cf:l:r:v:');

my $clist      = $opt_c || 0;
my $file       = $opt_f || 0;
my $var        = $opt_v || "cg"; #wlp, dp, lp, sz, md
my $lc         = $opt_l || 0;
my $rc         = $opt_r || 0;

#------------------------------------------------------------------------------

my $ws = $lc + $rc;
my $correct = "";
my $cgcnt = 0;
my %cg;
my %cd;
my %ic;
my %lex;

#--

open(FH, $file) || die "Can't open file.";

while ( my $line = <FH> ) {
  #print $line;
  if ( substr($line, 0, 1) eq "#" ) {
    next;
  }

  chomp $line;

  my @parts  = split (/ /, $line);
  if ( $#parts < 4 ) {
    next;
  }

  #  0,1,2   : context
  #  3 ws    : target
  #  4 ws+1  : prediction
  #  5 ws+2  : logprob of prediction
  #  6 ws+3  : entropy of distro returned (sum (p * log(p)) )
  #  7 ws+4  : word level entropy (2 ** -logprob) (from _word_ logprob)
  #  8 ws+5  : cg/cd/ic
  #  9 ws+6  : k/u
  # 10 ws+7  : md
  # 11 ws+8  : mal
  # 12 ws+9  : size of distro
  # 13 ws+10 : sumfreq distro
  # 14 ws+11 : MRR if file is .mg (compatible with .px)
  # 15 ws+12 : distribution
  #
  my $trgt = $parts[$ws];
  my $correct = $parts[$ws+5];
  
  #for (my $i = $ws+13; $i < $#parts; $i++ ) {
  #  print $parts[$i],"\n";
  #}

  if ( $#parts < $ws+13+3 ) {
    next;
  }

  # take the CD errors only
  if ( $correct ne "cd" ) {
    next;
  }

  # Number one in distro:
  my $cgw =  $parts[$ws+13 + 0];
  my $cgf = $parts[$ws+13 + 1];
  # Number two in distro:
  my $cdw =  $parts[$ws+13 + 2];
  my $cdf = $parts[$ws+13 + 3];

  # number two must be the classification (gone wrong)
  if ( $cdw ne $trgt ) {
    next;
  }

  my $ratio = $cdf * 100.0 / $cgf;
  #print $ratio," [",$cdw,"] should be [",$cgw,"]\n";

  $cd{$cdw}{$cgw} += 1; #do we check for reversed confusion also?

  #print $cgw,$cgf,$cdw,$cdf,"\n";

}

if ( $clist ) {
  # List for clist
  for my $key1 (sort keys %cd) {
    my $cnt = 0;
    for my $key2 (keys %{$cd{$key1}}) {
      $cnt += $cd{$key1}{$key2},;
    }
    if ( $cnt > 0 ) {
      my $nrs = 2;
      print $key1; 
      for my $key2 (sort {$cd{$key1}{$b} <=> $cd{$key1}{$a}} keys %{$cd{$key1}}) {
	print " ", $key2;
	if ( --$nrs <= 0 ) {
	  last;
	}
      }
      print "\n";
    }
  }
} else {
  for my $key1 (sort keys %cd) {
    my $cnt = 0;
    for my $key2 (keys %{$cd{$key1}}) {
      $cnt += $cd{$key1}{$key2},;
    }
    if ( $cnt > 0 ) {
      my $nrs = 3;
      print $key1, " should be";
      for my $key2 (sort {$cd{$key1}{$b} <=> $cd{$key1}{$a}} keys %{$cd{$key1}}) {
	print " ", $key2, " (", $cd{$key1}{$key2}, ") ";
	if ( --$nrs <= 0 ) {
	  last;
	}
      }
      print "\n";
    }
  }
}

