#!/usr/bin/perl -w
# $Id: $
#
use strict;
use Getopt::Std;

#------------------------------------------------------------------------------
# User options
#
# Cleaner version of check_spelerr.pl
#
#------------------------------------------------------------------------------

use vars qw/ $opt_a $opt_c $opt_n $opt_o $opt_s $opt_t $opt_v $opt_i/;

getopts('ac:o:ns:tv:i');

my $acc       = $opt_a || 0; #print accuracy, f-score, etc
my $no_output = $opt_n || 0; #produce no errs and errs1 output
my $orig_file = $opt_o || "";   #original instances (testfile)
my $sc_file   = $opt_s || "";   #wopr output
my $top_only  = $opt_t || 1;    #top-answer only HARDCODED
my $v         = $opt_v || 0;    #verbosity
my $conf_file = $opt_c || 0; #sets with confusibles, ignore others
my $do_info   = $opt_i || 0; #print label in normal mode.
my $oneliner = 1;

my %confusibles;

#------------------------------------------------------------------------------

# sc output:
#  third quarter profit (,) -10.1461 4.29174 1133.16 30 [ profits 1 ]
#  , the 10-year (company) -13.6654 9.18617 12993 1271 [ 30-year 1 15-year 1 ]
#
#  0 1   2  => instance from test_file

open(FHO, $orig_file) || die "Can't open file $orig_file.";
open(FHS, $sc_file)   || die "Can't open file $sc_file.";

my $out_data = $sc_file.".errs";
if ( $top_only ) {
  $out_data = $out_data."1";
}
if ( $no_output ) {
  open(OFHD, ">/dev/null") || die "Can't open datafile $out_data.";
} else {
  open(OFHD, ">$out_data") || die "Can't open datafile $out_data.";
}

if ( $conf_file ) {
  open(FHC, $conf_file) || die "Can't open file $conf_file.";
  while ( my $l = <FHC> ) {
	chomp $l;
	if ( substr($l, 0, 1) eq "#" ) {
	  next;
	}
	my @cs = split(/ /, $l);
	foreach (@cs) {
	  $confusibles{$_} = 1;
	  if ( $v > 0 ) {
		print "Adding: [$_]\n";
	  }
	}
  }
}
#Skip headers?

if ( $v > 0 ) {
  my $size = keys %confusibles;
  print "confusibles: $size\n";
}

my $l = 0; #position of (word)
my $c = 0; #position of bracket before first [ correction 1 ]

# TP, FP, TN, FN
my $good_sugg  = 0; # TP, corrected a real mistake
my $no_sugg    = 0; # ??,should have corrected a real mistake
my $bad_sugg   = 0; # tried to correct a mistake, got it wrong
my $wrong_sugg = 0; # TN, tried to correct a correct word.

my $lines      = 0;
my $clines     = 0; # lines, but when filtering on confusibles
my $errors     = 0;

#    utexas.10e6.dt3.t1e5.cf094.l2t1r2_CFEV87010.sc
# stronger by the amount of the (the) 0 0 0 0 [ ]
# by the amount of stem amount (number) -1.73599 0.881539 3.33108 2 [ number 2415 ]
#
#    utexas.10e6.dt3.t1e5.l2t1r2
# stronger by the amount of the
# by the amount of stem amount
#
while ( my $ls = <FHS> ) {

  ++$lines;

  my $lo = <FHO>; #No check if unequal lengths
  chomp $lo;
  my @po = split(" ", $lo);
  my $orig_target = $po[$#po]; #should be the correct word, from original

  chomp $ls;
  my @ps = split(" ", $ls);
 
  my $classification = "";
  my $i = 0;
  if ( $l == 0 ) { #do only once
    while ( $i < $#ps ) {
      if ( $ps[$i] =~ /\(.*\)/ ) { #the word between () is classification
        $l = $i;
		$classification = substr($ps[$i], 1, -1)
      }
      if ( $ps[$i] eq "[" ) {
        $c = $i;
        $i = $#ps;
      }
      ++$i;
    }
  } else { #we know where to look now
	if ( $ps[$l] =~ /\(.*\)/ ) { #the word between () is classification
	  $classification = substr($ps[$l], 1, -1);
	}
  }

  # If we zoom in on confusibles, we abort if the orig_target
  # is not in the confusibles.
  #
  if ( $conf_file ) {
	if (not exists $confusibles{$orig_target}) {
	  next;
	}
  }

  # Increment lines with confusible
  ++$clines;

  # l2r0:
  # could elect their (to) -8.32193 2.85737 320 53 [ the 9 this 2 other 1 ]

  my $test_target = $ps[ $l - 1 ]; #target in "spelerr"ed file, "their"
  my $corrections = $#ps - $c - 1; #position of words between [ ]
  
  my $have_corrections = 0;
  if ( substr($ls, -3) ne "[ ]" ) {
	$have_corrections = 1;
  }
  #print substr($ls, -3), $have_corrections;
  #print "\n";

  # Count "real" errors, number of differences between targets.
  #
  my $text_error = 0;
  if ( $test_target ne $orig_target ) { #then error in the text
    ++$errors; #in the text
    $text_error = 1; #error with respect to gold data
	if ( $v > 0 ) {
	  print "[$test_target] (real error, should be [$orig_target])\n";
	}
    print OFHD "ERROR: $test_target should be $orig_target\n";
  }


  if ( $text_error > 0 ) {   # We have an error in the text
	
	if ( $have_corrections > 0 ) { # we have a wopr correction

	  if ( $classification eq $orig_target ) { # and wopr got it correct
		++$good_sugg;
		print OFHD "GOODSUGG $test_target -> $classification\n";
	  } else {  # we tried to correct, but got it wrong
		++$wrong_sugg;
		print OFHD "WRONGSUGG $test_target -> $classification (should be: $orig_target)\n";
	  }
	} else { # we did not have a correction, we missed an error
	  ++$no_sugg;
	  print OFHD "NOSUGG $test_target (should be: $orig_target)\n";
	}
  	
  } #end if text_error
  
  if ( $text_error == 0 ) { #there was no error in the text

	if ( $have_corrections > 0 ) { # but we do have a suggestion from wopr
	  if ( $classification eq $orig_target ) { # and wopr got it correct
		# ignore, we concentrate on errors, a bad_suggestion is a bad suggestion however
	  } else { #wopr turned a correct confusible in an incorrect one
		++$bad_sugg;
		print OFHD "BADSUGG $test_target -> $classification (should be: $orig_target)\n";
	  }
	}

  } #end if no_error

}
close(OFHD);
close(FHS);
close(FHO);

my $TP = $good_sugg;
my $FN = $wrong_sugg + $no_sugg;
my $FP = $bad_sugg;
my $TN = $clines - $TP - $FN - $FP; #Niks gedaan als niks moest

my $RCL = 0;
if ( $TP+$FN > 0 ) {
  $RCL = ($TP*100)/($TP+$FN);           #TPR
}
my $ACC = (100*($TP+$TN))/($clines);

my $PRC = 0;
if ( $TP+$FP > 0 ) {
  $PRC = (100*$TP)/($TP+$FP);           #PPV
}
my $FPR = 0;
if ( $FP+$TN > 0 ) {
  $FPR = (100*$FP)/($FP+$TN);
}
my $F1S = (100*2*$TP)/((2*$TP)+$FP+$FN);

if ( $do_info ) {
  printf( "lines  clines   errs    GS    WS    NS    BS    TP    FP    FN    TN    ACC PRE REC F1S\n" );
}
printf( "%6i %6i %6i %5i %5i %5i %5i %5i %5i %5i %5i %5.2f %5.2f %5.2f %5.2f\n", $lines, $clines, $errors, $good_sugg, $wrong_sugg, $no_sugg, $bad_sugg, $TP, $FP, $FN, $TN, $ACC, $PRC, $RCL, $F1S);

__END__

