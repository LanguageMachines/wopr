#!/usr/bin/perl -w
# $Id:$
#
use strict;
use Getopt::Std;

#------------------------------------------------------------------------------
#
# Takes a DATA...plot file, creates tables with gs/ws etc scores.
# Only works with newest versions which include HPX setting in output.
#
#------------------------------------------------------------------------------

use vars qw/ $opt_f $opt_r /;

getopts('f:r:');

my $data_file = $opt_f || "";   #original data file, two lines (normal TOP1) 
my $re        = $opt_r || ".*"; #regular expression to match lines

#------------------------------------------------------------------------------

# TPO11003 100000 18370 350 181 333 402 162 51.71 95.14 114.86 46.29 l4r2_-a1+D hpx
# TPO11003 100000 18370 350 170 112 18 162 48.57 32.00 5.14 46.29 l4r2_-a1+D hpx  TOP1
#                                                              sometimes md5sum ^  
# Sometimes without                        ^^^^^^^^^^^^^^^^^^^^^^ these

# indices to data line
my $gs = 4;
my $ws = 6;
my $ns = 7;
my $bs = 5;
my $algoffset = 3;

open(FHD, $data_file) || die "Can't open file.";

while ( my $ls0 = <FHD> ) {
  my $ls1 = <FHD>;

  #print $ls0;
  #print $ls1;

  if ( $ls1 =~ m/$re/ ) {
    chomp $ls0;
    my @l0 = split(/\ /, $ls0);
    my $numl0 = $#l0;
    chomp $ls1;
    my @l1 = split(/\ /, $ls1);
    my $numl1 = $#l1;

    my $alg = $l1[ $numl1 - $algoffset ]; #could be empty if missing md5sum
    #print "{".$alg."}\n";
    if ($alg eq "") {
      my $alg = $l1[ $numl1 - ($algoffset+1) ];
    }
    $alg =~ s/_\-a1\+D/ \\igtree{}/;
    $alg =~ s/_\-a4\+D/ \\triblt{}/;

    my $sum_gswsns = $l1[$gs]+$l1[$ws]+$l1[$ns];

    my $out = "\\cmp{".$alg."} & \\num{".$l0[3]."} & \\num{".$l0[$gs]."} & \\num{";
    $out = $out . $l1[$gs]."} & \\num{".$l1[$ws]."} & \\num{".$l1[$ns]."} & \\num{".$l1[$bs]."} \\\\";
    $out = $out . " %".$l1[0];
    print $out,"\n";
  }
}
close(FHD);


__END__

