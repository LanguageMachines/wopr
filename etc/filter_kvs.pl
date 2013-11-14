#!/usr/bin/perl -w
# $Id$
#
use strict;
use Getopt::Std;

#------------------------------------------------------------------------------
# User options
#
# Filter the kvs list to include only the classifiers in the supplied list.
#
#------------------------------------------------------------------------------

use vars qw/ $opt_d $opt_k $opt_l /;

getopts('d:k:l:');

my $default   = $opt_d || "dflt";
my $list_file = $opt_l || 0;
my $kvs_file  = $opt_k || 0;

#------------------------------------------------------------------------------

my %list;

open(FH, $list_file) || die "Can't open list file.";
while ( my $line = <FH> ) {
  chomp $line;
  $list{$line} = 1;
}
close(FH);

if ( $default ) {
  $list{$default} = 1;
}

open(FH, $kvs_file) || die "Can't open kvs file.";
my $keep_cl = 0;
while ( my $line = <FH> ) {
  chomp $line;

  if ( ( $line ne "" ) && ($line =~ /^classifier:/ )) {

    my @parts = split(/:/, $line, 2); # problem with "classifier::"
    my $cl_name = $parts[1];

    if ( defined $list{$cl_name} ) {
      $keep_cl = 1;
    } else {
      $keep_cl = 0;
    }
  }
  if ( $keep_cl ) {
    print "$line\n";
  }
}

close(FH);
  
__END__

