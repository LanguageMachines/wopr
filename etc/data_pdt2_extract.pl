#!/usr/bin/perl -w
#
use strict;
use Getopt::Std;
#
#------------------------------------------------------------------------------
# User options
# PJB: better to select data to PRE format, and plot that with the
# script to create plots for the PRE data...?
#
# perl ~/uvt/wopr/etc/data_pdt2_extract.pl -f DATA.PDTT.10000.plot
# more OUT_10000_l8_-a4+D_511.data
# PDTT10018 1000 136754 54462 39.8248 3 511 l2r0_-a1+D
# PDTT10042 5000 136754 54963 40.1911 3 511 l2r0_-a1+D
# PDTT10018 1000 136754 54462 39.8248 3 511 l2r0_-a1+D
# ...
# PDTT10210 10000000 136754 69327 50.6947 3 511 l2r0_-a1+D
#
# We also have OUT_10000_l2_-a4+D_511.data
#                        ^^ these can be included in plot
#
# combine_plot.sh file0 file1 ... fileN
#
#------------------------------------------------------------------------------

use vars qw/ $opt_f $opt_o $opt_r $opt_t /;

getopts('f:o:rt:');

my $file       = $opt_f || 0;
my $outbase    = $opt_o || "OUT";
my $reportonly = $opt_r || 0;
my $type       = $opt_t || "c"; # c=combined, l=letters, w=words

open(FH, $file) || die "Can't open file.";

my %lines0;
my %lines1;
my %lds;    #letter depthsx
my %lcs0;   #letter contexts
my %algos0; #letter algos
my %ltfull; #full letter context
my %dss;    #word cntext ds values
my %algos1; #word algos
my %wfull; #full word context

while ( my $line = <FH> ) {
  if ( substr($line, 0, 1) eq "#" ) {
    next;
  }

# PDTT10008 10000 1000 136754 60301 44.0945 51031 37.3159 9270 6.7786 3 3 511 l12_-a1+D l2_-a1+D
# PDTT10008 10000 1000 136754 60301 44.0945 51031 37.3159 9270 6.7786 3 3 511 c12_-a1+D l2_-a1+D
#                                                                           !!^!!
# 0         1     2    3      4     5       6     7       8    9      
#
# PRE10008 1000 136754 9851 7.20345 3 555 l3r0_-a1+D
#
  chomp $line;
  #$line =~ s/_/\\_/g;

  my @parts  = split (/ /, $line);
  if ( $#parts < 4 ) {
    next;
  }

  $lines0{ $parts[1] } = 1;
  $lines1{ $parts[2] } = 1;

  $ltfull{ $parts[13] } = 1; #The whole letter context, used in filename
  my ($lc, $algo0) = split("_", $parts[13]);
  $lcs0{ $lc } = 1;
  $algos0{ $algo0 } = 1;

  $wfull{ $parts[14] } = 1; #The whole word context, used in filename
  my ($wc, $algo1) = split("_", $parts[14]);
  #$lcs0{ $wc } = 1;#not saved at the moment
  $algos1{ $algo1 } = 1;

  $lds{ $parts[10] } = 1;
  $dss{ $parts[12] } = 1;
}

if ( $reportonly ) {
  print "LINES(lc): ";
  foreach my $key (sort { $a <= $b } (keys(%lines0))) {
    #print $key." ".$lines0{$key}."\n";
    print $key." ";
  }
  print "\n";

  print "LINES(wc): ";  
  foreach my $key (sort { $a <= $b } (keys(%lines1))) {
    #print $key." ".$lines1{$key}."\n";
    print $key." ";
  }
  print "\n";

  print "DEPTH(lc): ";
  foreach my $key (sort { $a gt $b } (keys(%lds))) {
    print $key." ";
  }
  print "\n"; 

  print "CTX(lc): "; 
  foreach my $key (sort { $a gt $b } (keys(%lcs0))) {
    print $key." ";
  }
  print "\n";

  print "ALGO(lc): ";
  foreach my $key (sort { $a gt $b } (keys(%algos0))) {
    print $key." ";
  }
  print "\n";

  print "ALGO(wc): ";
  foreach my $key (sort { $a gt $b } (keys(%algos1))) {
    print $key." ";
  }
  print "\n";

  print "DEPTHS(wc): ";
  foreach my $key (sort { $a gt $b } (keys(%dss))) {
    print $key." ";
  }
  print "\n";
  exit;
}

#### Pass TWO 

my $OFFtype = "notltf"; #nope

my @l = (0,2,3,4,5,11,12,14); #field indexes to print
#              ^ ^ = absolute/percentage
# can also be  6,7 or 8,9 for
# letter only or word only
#
if ( $type eq "l" ) { #letters
  @l = (0,2,3,6,7,11,12,14);
} elsif ( $type eq "w" ) { #word scores
  @l = (0,2,3,8,9,11,12,14);
}

foreach my $l0 (keys(%lines0)) {
  foreach my $ld (keys(%lds)) {
    foreach my $ltf (keys(%ltfull)) { 
      foreach my $wf (keys(%wfull)) { 
	foreach my $ds (keys(%dss)) {
      
	  my $lines = 0;
	  my %ulines; #uniq lines only (ieks)

	  # Depending on context/graph kind/...
	  my $out = "ERROR.data";
	  my @tsts = (1,10,12,13,14); #those five loops
	  if ( $OFFtype eq "ltf" ) {
	    $out = $outbase."_".$l0."_".$ds.".data";
	    @tsts = (1,12);
	  } else {
	    $out = $outbase."_".$l0."_".$ld."_".$ltf."_".$ds."_".$wf.".data";
	  }
      
	  my @tstdata;
	  $tstdata[1]  = $l0;
	  $tstdata[10] = $ld;
	  $tstdata[12] = $ds;
	  $tstdata[13] = $ltf;
      	  $tstdata[14] = $wf;

	  print STDERR "$out $l0 $ld $ltf $wf $ds\n";
	  open(OFH, ">$out") || die "Can't open: $out";
      
	  # Goto beginning of file.
	  seek FH,0,0;
      
	  while ( my $line = <FH> ) {
	    if ( substr($line, 0, 1) eq "#" ) {
	      next;
	    }
	    chomp $line;
	    my @parts  = split (/ /, $line);
	    if ( $#parts < 4 ) {
	      next;
	    }

	    # Create/test unique lines
	    my $uline = "";
	    foreach my $l (@l) {
	      if ( $l > 0 ) { #filter ID (maybe also scores?)
		$uline = $uline . $parts[$l] . " ";
	      }
	    }
	    if ( defined $ulines{$uline} ) {
	      next; #it existed, skip.
	    }
	    $ulines{$uline} = 1;

	    # Select with 10000 lines letters, l12_+a1+D letter context, etc
	    # some kind of eval/loop?
	    #
	    my $match = 1;
	    foreach my $tst (@tsts) {
	      if ( $parts[$tst] ne $tstdata[$tst] ) {
		$match = 0;
		#exit loop
	      }
	    }
	    if ( $match == 1 ) {
	      # call it DATA_..._10000_l12_-a1+D_... ?
	      # contents: PDTT10080 50000 136754 64001 46.8001 3 511 l2_-a1+D
	      #
	      foreach my $l (@l) {
		if ( $l == 14 ) { # create mega algo string
		  my ($lc, $algo1) = split("_", $parts[$l]);
		  if ( $OFFtype eq "ltf" ) {
		    print OFH "$lc"."r0_".$algo1."_".$ltf;
		  } else {
		    print OFH "$lc"."r0_".$algo1; #."_".$ltf;
		  }
		} else {
		  print OFH $parts[$l]." ";
		}
	      }
	      print OFH "\n";
	      ++$lines;
	    }
	  }
	  close(OFH);
	  if ( $lines == 0 ) {
	    print STDERR "EMPTY(".$out."), removing.\n";
	    unlink $out;
	  }
	}			#dss
      }				#lc0
    }				#wf
  }				#ltf
}				#l0

# ----

sub num {
  my $n = shift;
  my $d = shift;

  print "& \\num{".sprintf( $d, $n)."} ";
}

sub str {
  my $n = shift;
  my $d = "%s";

  print "& ".sprintf( $d, $n)." ";
}
