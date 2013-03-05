#!/usr/bin/perl -w
#
use strict;
use Getopt::Std;
#
#------------------------------------------------------------------------------
# User options
#------------------------------------------------------------------------------

use vars qw/ $opt_f $opt_d /;

getopts('f:d:');

my $errfile = $opt_f || "";
my $dtifile = $opt_d || "";

my %errs;

open(FHE, $errfile) || die "Can't open file.";

while ( my $line = <FHE> ) {
  chomp $line;

  if ( $line =~ / / ) { #skip entries with spaces
    next;
  }

  my @parts  = split (/~/, $line);
  push @{ $errs{$parts[0]} }, $parts[1];
}
close( FHE );

#print @{$errs{"met"}};

my $niks = 0;
my $repl = 0;

open(FH, $dtifile) || die "Can't open file.";

while ( my $line = <FH> ) {
  chomp $line;

  my @posserrs; #list with possible errors

  foreach my $k (keys %errs)  {
    my $w = " $k ";
    #begin/end is tricky, only take in between
    #end is always punctuation anyway
    #my $w0 = "^$k";
    #my $w1 = "$k\$";
    
    #if ( $line =~ /($w|$w0|$w1)/ ) {
    if ( $line =~ /$w/ ) {
      #my $num = scalar @{ $errs{$k} }; #number of misspellings for this word
      #my $idx = int(rand($num));
      #print "$k, $num, ", $errs{$k}[$idx], "\n";
      #print $k, " ", scalar @{ $errs{$k} }, "\n";
      #print "$line\n";
      push @posserrs, $k;
    }
  }

  print STDERR "$line\n";
  if ( scalar @posserrs == 0 ) {
    print STDERR "niks\n";
    print "$line\n"; #one without errors.
    ++$niks;
    next;
  }

  #print join( ",", @posserrs ), "\n";
  my $idx_posserr = int(rand(scalar(@posserrs)));
  my $posserr = $posserrs[$idx_posserr]; #choose one
  #print "$posserr ", scalar @{ $errs{$posserr} }; #this many possible misspellings
  my $subs_idx = int(rand(@{ $errs{$posserr} })); #choose a misspelling
  my $newerr = $errs{$posserr}[$subs_idx];
  $line =~ s/ $posserr / $newerr /; #introduce error (only the first match)
  ++$repl;
  print "$line\n";
  print STDERR "$posserr - $newerr\n";
  print STDERR "\n";
}
close( FH );

print STDERR "$repl - $niks\n";
