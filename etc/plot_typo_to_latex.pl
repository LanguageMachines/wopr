#!/usr/bin/perl -w
# $Id:$
#
use strict;
use Getopt::Std;

#------------------------------------------------------------------------------
#
# Takes a DATA...plot file, creates tables with gs/ws etc scores.
#
#------------------------------------------------------------------------------

use vars qw/ $opt_f /;

getopts('f:');

my $data_file = $opt_f || "";   #original data file, two lines (normal TOP1) 


#------------------------------------------------------------------------------

# TPO11003 100000 18370 350 181 333 402 162 51.71 95.14 114.86 46.29 l4r2_-a1+D 
# TPO11003 100000 18370 350 170 112 18 162 48.57 32.00 5.14 46.29 l4r2_-a1+D  TOP1
#                                                           sometimes md5sum ^  
# Sometimes without                        ^^^^^^^^^^^^^^^^^^^^^^ these

open(FHD, $data_file) || die "Can't open file.";

while ( my $ls0 = <FHD> ) {
  my $ls1 = <FHD>;

  #print $ls0;
  #print $ls1;

  chomp $ls0;
  my @l0 = split(/\ /, $ls0);
  my $numl0 = $#l0;
  chomp $ls1;
  my @l1 = split(/\ /, $ls1);
  my $numl1 = $#l1;

  my $alg = $l1[ $numl1 - 2 ]; #could be space if missing md5sum
  #print "{".$alg."}\n";
  if ($alg eq "") {
    my $alg = $l1[ $numl1 - 3 ];
  }
  $alg =~ s/_\-a1\+D/ \\igtree{}/;
  $alg =~ s/_\-a4\+D/ \\triblt{}/;

  my $out = "\\cmp{".$alg."} & \\num{".$l0[3]."} & \\num{".$l0[4]."} & \\num{";
  $out = $out . $l1[4]."} & \\num{".$l1[6]."} & \\num{".$l1[7]."} & \\num{".$l1[5]."} \\\\";
  $out = $out . " %".$l1[0];
  print $out,"\n";
}
close(FHD);


__END__

