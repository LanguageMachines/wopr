#!/usr/bin/perl -w
# $Id$
#
use strict;
use Getopt::Std;

#------------------------------------------------------------------------------
# User options
#
# Check correct output.
#
# Takes the .sc output from wopr, tested on a "modified" test file with
# make_spelerr.pl. Output is compared to the original, unmodified
# test file used to create the file with errors. Computes statistics.
#
# ../wopr -r correct -p ibasefile:rmt.1e5.l2r0_-a1+D.ibase,lexicon:rmt.1e5.lex,
#    timbl:"-a1 +D",filename:tmp,max_ent:10000,max_distr:10000
#
# for file in `ls tmp*sc`; do echo $file;
#   perl ../etc/check_spelerr.pl -s $file -o rmt.t1000.l2r0;
# done
#
#durian:doc pberck$ perl ../etc/check_spelerr.pl -o rmt.t1000.l2r0 -s rmt.t1000err.l2r0_EXP02.sc 
# lines:      16358
# errors:     1218
# good_sugg:  28
# bad_sugg:   21
# wrong_sugg: 0
# no_sugg:    1190
#durian:doc pberck$ perl ../etc/check_spelerr.pl -o rmt.t1000.l2r0 -s rmt.t1000err.l2r0_EXP01.sc 
# lines:      16358
# errors:     1218
# good_sugg:  21
# bad_sugg:   20
# wrong_sugg: 0
# no_sugg:    1197
#
#------------------------------------------------------------------------------

use vars qw/ $opt_a $opt_c $opt_n $opt_o $opt_s $opt_t $opt_v/;

getopts('ac:o:ns:tv:');

my $acc       = $opt_a || 0; #print accuracy, f-score, etc
my $no_output = $opt_n || 0; #produce no errs and errs1 output
my $orig_file = $opt_o || "";   #original instances (testfile)
my $sc_file   = $opt_s || "";   #wopr output
my $top_only  = $opt_t || 0;    #top-answer can be correct only 
my $v         = $opt_v || 0;    #verbosity
my $conf_file = $opt_c || 0; #sets with confusibles, ignore others
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
	}
  }
}
#Skip headers?

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

while ( my $ls = <FHS> ) {

  ++$lines;

  my $lo = <FHO>; #No check if unequal lengths
  chomp $lo;
  my @po = split(/ /, $lo);
  my $orig_target = $po[$#po]; #should be the correct word

  chomp $ls;
  my @ps = split(/ /, $ls);
 
  my $i = 0;
  if ( $l == 0 ) { #do only once
    while ( $i < $#ps ) {
      if ( $ps[$i] =~ /\(.*\)/ ) { #the word between ()
        $l = $i;
      }
      if ( $ps[$i] eq "[" ) {
        $c = $i;
        $i = $#ps;
      }
      ++$i;
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

  ++$clines;

  # l2r0:
  # could elect their (to) -8.32193 2.85737 320 53 [ the 9 this 2 other 1 ]

  my $test_target = $ps[ $l - 1 ]; #target in "spelerr"ed file.
  my $corrections = $#ps - $c - 1; #position of words between [ ]

  # Count "real" errors, number of differences between targets.
  #
  my $text_error = 0;
  if ( $test_target ne $orig_target ) {
    ++$errors;
    $text_error = 1; #error with respect to gold data
    print OFHD "ERROR: $test_target should be $orig_target\n";
  }

  # Do we have suggestions from wopr?
  #
  if ( $corrections > 0 ) {
    #print "$test_target/$orig_target: $l $c $corrections\n";
    if ( $v > 0 ) {
      # There is a difference between the targets, i.e an error.
      print "$test_target (was $orig_target)\n";
    }
    my $index_counter = 0;
    for ( my $idx = $c+1; $idx < $#ps; $idx += 2) {
      my $correction = $ps[$idx];
      #
      # If one of the suggestions equals $orig_target, we did
      # a correct spelling correction correction. :-) In strict
      # mode, only the top answer can be correct.
      #
      if ( $correction eq $orig_target ) {
        if ( $v > 0 ) {
          print "$test_target -> $correction GOODSUGG\n";
        }
        print OFHD "GOODSUGG $test_target -> $correction\n";
        ++$good_sugg;
      } else {
    		#check for unneccessary corrections also
		    # could be a mistake, corrected wrongly, or no mistake to start with.
		    #   states -> status (should be: states) BADSUGG
		    # versus
		    #   Austraia -> Austria (should be: Australia) WRONGSUGG
		  if ( $text_error == 0 ) { #no text error
        if ( $v > 0 ) {
			    # here we suggest a correction for something that is not wrong.
          print "$test_target -> $correction (should be: $orig_target) BADSUGG\n";
        }
        print OFHD "BADSUGG $test_target -> $correction (should be: $orig_target)\n";
        ++$bad_sugg;
      } else { #we do have a text error
        if ( $v > 0 ) {
			    # here we suggest the wrong correction for an error.
          print "$test_target -> $correction (should be: $orig_target) WRONGSUGG\n";
        }
        print OFHD "WRONGSUGG $test_target -> $correction (should be: $orig_target)\n";
        ++$wrong_sugg;
      } #else
    }
    ++$index_counter;
    if ( $top_only ) {
      last; # only do the first, break to ignore the rest.
    }
  }
} # corrections > 0
  # No suggestions, but we had an error.
  #
  if ( ($corrections == 0) && ($test_target ne $orig_target) ) {
    if ( $v > 0 ) {
      # we should have suggested a correction.
      print "$test_target (was $orig_target) NOSUGG\n";
    }
	print OFHD "NOSUGG $test_target (should be: $orig_target)\n";
    ++$no_sugg;
  }
  
}
close(OFHD);
close(FHS);
close(FHO);

if ( $acc ) {
  #Global, same as correctie
  my $TP = $good_sugg;
  my $FN = $wrong_sugg + $no_sugg;
  my $FP = $bad_sugg;
  #my $TN = $lines - $errors - $bad_sugg; #Niks gedaan als niks moest
  my $TN = $clines - $TP - $FN - $FP; #Niks gedaan als niks moest

  #
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

  #my $F1Sp = 2*( ($PRC*$RCL)/($PRC+$RCL) );
  #print STDERR $F1S,"  ",$F1Sp, "\n";

  #Detectie:
  my $dTP = $good_sugg + $wrong_sugg;
  my $dFN = $no_sugg;
  my $dFP = $bad_sugg;
  #my $dTN = $clines - $errors - $bad_sugg; #niks gedaan als niks moest
  my $dTN = $clines - $dTP - $dFN - $dFP;

  my $dRCL = 0;
  if ($dTP+$dFN > 0) {
    $dRCL = ($dTP*100)/($dTP+$dFN);           #TPR
  }
  my $dACC = (100*($dTP+$dTN))/($clines);
  my $dPRC = 0;
  if ( $dTP+$dFP > 0 ) {
    $dPRC = (100*$dTP)/($dTP+$dFP);          #PPV
  }
  my $dFPR = 0;
  if ( $dFP+$dTN > 0 ) {
    $dFPR = (100*$dFP)/($dFP+$dTN);
  }

  my $dF1S = (100*2*$dTP)/((2*$dTP)+$dFP+$dFN);

  #Correctie:
  my $cTP = $good_sugg;
  my $cFN = $no_sugg + $wrong_sugg;
  my $cFP = $bad_sugg;
  #my $cTN = $clines - $errors - $bad_sugg; #niks gedaan als niks moest
  my $cTN = $clines - $cTP - $cFN - $cFP;

  my $cRCL = 0;
  if ( $cTP+$cFN > 0 ) {
    $cRCL = ($cTP*100)/($cTP+$cFN);           #TPR
  }
  my $cACC = (100*($cTP+$cTN))/($clines);
  my $cPRC = 0;
  if ( $cTP+$cFP > 0 ) {
    $cPRC = (100*$cTP)/($cTP+$cFP);
  }
  my $cFPR = 0;
  if ( $cFP+$cTN > 0 ) {
    $cFPR = (100*$cFP)/($cFP+$cTN);
  }
  my $cF1S = (100*2*$cTP)/((2*$cTP)+$cFP+$cFN);

  #print "a TP+FN=",$TP+$FN,", FP+TN=", $FP+$TN,", TP+FP=", $TP+$FP,",FN+TN=", $FN+$TN, "\n";
  #print "d TP+FN=",$dTP+$dFN,", FP+TN=", $dFP+$dTN,", TP+FP=", $dTP+$dFP,",FN+TN=", $dFN+$dTN, "\n";
  #print "c TP+FN=",$cTP+$cFN,", FP+TN=", $cFP+$cTN,", TP+FP=", $cTP+$cFP,",FN+TN=", $cFN+$cTN, "\n";
  my $out = sprintf("%i %i %i %i %.2f %.2f %.2f %.2f", $TP, $FN, $FP, $TN, $PRC, $RCL, $ACC, $F1S);
  my $dout = sprintf("%i %i %i %i %.2f %.2f %.2f %.2f", $dTP, $dFN, $dFP, $dTN, $dPRC, $dRCL, $dACC, $dF1S);
  my $cout = sprintf("%i %i %i %i %.2f %.2f %.2f %.2f", $cTP, $cFN, $cFP, $cTN, $cPRC, $cRCL, $cACC, $cF1S);
  #print "a $clines $errors $good_sugg $bad_sugg $wrong_sugg $no_sugg "; 
  #print "$out\n";
  print "$lines $errors $good_sugg $bad_sugg $wrong_sugg $no_sugg "; #Here be $lines 
  print "d $dout "; #detection
  print "c $cout\n"; #correction

} elsif ($oneliner) {
  #print "l:$clines e:$errors gs:$good_sugg bs:$bad_sugg ws:$wrong_sugg ns:$no_sugg\n";
  print "$lines $errors $good_sugg $bad_sugg $wrong_sugg $no_sugg ";
  if ($errors != 0) {
   my $out = sprintf("%.2f %.2f %.2f %.2f", ($good_sugg/$errors*100),($bad_sugg/$errors*100),($wrong_sugg/$errors*100),($no_sugg/$errors*100));
   print "$out\n";
  } else {
   print "\n";
  }
} else {
  print "lines:      $lines\n";
  print "errors:     $errors\n";
  print "good_sugg:  $good_sugg\n";
  print "bad_sugg:   $bad_sugg\n";
  print "wrong_sugg: $wrong_sugg\n";
  print "no_sugg:    $no_sugg\n";
}

__END__

