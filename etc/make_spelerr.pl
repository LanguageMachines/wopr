#!/usr/bin/perl -w
# $Id:$
#
use strict;
use Getopt::Std;

#------------------------------------------------------------------------------
# User options
#
# Introduce "spelling errors".
#------------------------------------------------------------------------------

use vars qw/ $opt_f $opt_l $opt_m $opt_p /;

getopts('f:l:m:p:');

my $file    = $opt_f || "";
my $lexicon = $opt_l || "";
my $mode    = $opt_m || 1;   #0 is instances, 1 is plain text
my $p       = $opt_p || 0.9; #prob for a change

#------------------------------------------------------------------------------

my %lex;
if ( $lexicon ne "" ) {
  open(FHW, $lexicon)  || die "Can't open file.";
  while ( my $line = <FHW> ) {
    chomp $line;

    my @parts  = split(/ /, $line);

    $lex{$parts[0]} = $parts[1];
  }
  close(FHW);
}

open(FH, $file) || die "Can't open file.";
while ( my $line = <FH> ) {

  chomp $line;

  my @parts = split(/ /, $line);

  if ( $mode == 0 ) {
    my $last_pos = $#parts;
    my $target   = $parts[$last_pos];
    
    my $lt = length($target);
    if ( $lt > 1 ) { #skip _ . , etc
      if ( rand() > $p ) {
       $target = errorify( $target );
      }
    }
    print join(' ', @parts[0..($#parts-1)], $target), "\n";
  } elsif ( $mode == 1 ) {
    foreach my $word ( @parts ) {
      if ( rand() > $p ) {
       $word = errorify( $word );
      }
      print "$word ";
    }
    print "\n";
  }
  
}
close(FH);

sub errorify {
    my $target = shift || "ERROR"; 

    # mangle target
    my $r = int(rand(4));
    my $lt = length($target);
    if ( $lt < 3 ) { #ignore very small words
      return $target;
    }
    if ( $r == 0 ) {
      #Swap two consecutive letters, LD 1
      my $idx = int(rand($lt-1));
      my @chars = split ( //, $target );
      my $tmp = $chars[$idx];
      $chars[$idx] = $chars[$idx+1];
      $chars[$idx+1] = $tmp;
      $target = join( '', @chars );
      #print "$target $lt $idx\n";
    } elsif ( $r == 1 ) {
      #Delete a random letter, LD 1
      my $idx = int(rand($lt));
      my @chars = split ( //, $target );
      $target = join( '', @chars[0..$idx-1], @chars[$idx+1..($#chars)] );
      #print "DELETE($idx): $target\n";
    } elsif ( $r == 2 ) {
      #Swap two non-consecutive letters, LD 2 (could be 0)
      my $idx0 = int(rand($lt));
      my $idx1 = int(rand($lt));
      my @chars = split ( //, $target );
      my $tmp = $chars[$idx0];
      $chars[$idx0] = $chars[$idx1];
      $chars[$idx1] = $tmp;
      $target = join( '', @chars );
      #print "$target $lt $idx\n";
    } elsif ( $r == 3 ) {
      #Duplicate a random letter, LD 1
      my $idx = int(rand($lt));
      my @chars = split ( //, $target );
      $target = join( '', @chars[0..$idx], @chars[$idx..($#chars)] );
      #print "DUPLICATE($idx): $target\n";
    }
  
    if ( exists $lex{$target}) {
      print "(e)";
    }
  return $target; # check if in lexicon
}
 
__END__

