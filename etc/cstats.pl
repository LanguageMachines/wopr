#!/usr/bin/perl -w
#
use strict;
use Getopt::Std;
#
#------------------------------------------------------------------------------
# User options
#------------------------------------------------------------------------------

use vars qw/ $opt_f /;

getopts('f:');

my $file = $opt_f || 0;

open(FH, $file) || die "Can't open file.";

my $lines = 0;
my $words = 0;
my $chars = 0;

while ( my $line = <FH> ) {

  chomp $line;

  my @parts  = split (/ /, $line);
  my $numparts = $#parts+1;

  ++$lines;
  $words += $numparts;

  #print "$numparts\n";

  for ( my $i = 0; $i < $numparts; $i++ ) {
    #print $i, " ", length($parts[$i]), "\n";
    $chars += length($parts[$i]);
  }
  
}

print "$lines $words $chars\n";

seek FH,0,0;

my $ave_ll = $words / $lines; #average lines length, in words
my $ave_wl = $chars / $words; #average word length, in chars

print "ave_ll: ", $ave_ll, "\n";
print "ave_wl: ", $ave_wl, "\n";

#my $ldiffs = 0; #line diff squared
my $wdiffs = 0; #word diff squared, for line length
my $cdiffs = 0; #word diff squared, for word length

while ( my $line = <FH> ) {

  my @parts  = split (/ /, $line);
  my $numparts = $#parts+1;

  my $df = ($ave_ll - $numparts);
  $wdiffs = $wdiffs + ($df * $df);

  for ( my $i = 0; $i < $numparts; $i++ ) {
    my $l = length($parts[$i]);
    my $foo = ($ave_wl - $l);
    $cdiffs = $cdiffs + ($foo * $foo);
  }
  
}

print "std.dev line length = ", sqrt($wdiffs / ($lines-1)), "\n";
print "std.dev word length = ", sqrt($cdiffs / ($words-1)), "\n";

__END__
