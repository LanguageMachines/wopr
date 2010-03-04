#!/usr/bin/perl -w
# $Id: pplx_px.pl 3746 2010-02-17 10:44:20Z pberck $
#
use strict;
use Getopt::Std;

#------------------------------------------------------------------------------
# User options
# ...
#------------------------------------------------------------------------------

use vars qw/ $opt_f $opt_i /;

getopts('f:i');

my $wopr_file  = $opt_f || 0;
my $ignore_oov = $opt_i || 0;

#------------------------------------------------------------------------------

my %summary;

my $word_pos    = 0;
my $target_pos  = 0;
my $prob_pos    = 1;
my $n1_pos      = 3; # first word of ngram

my $wopr_sumlog10  = 0.0;
my $srilm_sumlog10 = 0.0;
my $wordcount = 0;
my $sentencecount = 0;
my $oovcount = 0;

my $f = "%01b"; # Number of binary indicators

open(FHW, $wopr_file)  || die "Can't open file.";

while ( my $line = <FHW> ) {

    if ( substr($line, 0, 1) eq "#" ) {
      next;
    }
    if ( substr($line, 0, 3) eq "<s>" ) {
      next; # Postproc hack to get sililar values to SRILM
    }
    chomp $line;

    my @parts = [];
    my $target;
    my $wopr_log2prob;
    my $wopr_log10prob;
    my $wopr_prob;
    my $classtype;
    my $icu;
    my $md;
    my $ml;
    my $extra = 0;
    my $indicators = 0;
    
    @parts  = split (/ /, $line);
    $target         = $parts[ $target_pos ];
    $wopr_prob      = $parts[ $prob_pos ];
    $wopr_log2prob  = log2( $wopr_prob );
    $wopr_log10prob = log10( $wopr_prob );

    $icu = $parts[ $n1_pos ];

    if ( $target eq "<\/s>" ) {
      ++$sentencecount;
    } else {
      ++$wordcount;
    }
    if ( $icu eq "OOV" ) {
      $indicators += 1;
      ++$oovcount;
    }

    if ( ($icu ne "OOV") && ($wopr_prob != 0) ) {
      $wopr_sumlog10 += $wopr_log10prob;
    }

    printf( "%.8f %8.4f %8.4f ", $wopr_prob, $wopr_log2prob, $wopr_log10prob );

    #
    printf( "[$f] ", $indicators );
    print "$target";
    print "\n";
    ++$summary{$indicators};
  } 


my $tot = 0;
foreach my $key (sort (keys(%summary))) {
  $tot += $summary{$key};
}
foreach my $key (sort { $a <=> $b } (keys(%summary))) {
  printf( "$f:%6i (%6.2f%%)\n", $key, $summary{$key},  $summary{$key}*100/$tot );
}

print "\n";
print "Sum log10 probs:\n";
printf( "Wopr:  %8.2f\n", $wopr_sumlog10 );
printf( "Wordcount: %i sentencecount: %i oovcount: %i\n", 
	$wordcount, $sentencecount, $oovcount );
if ( ! $ignore_oov ) {
  printf( "Wopr ppl:  %8.2f ", 10 ** (-$wopr_sumlog10/($wordcount-$oovcount+$sentencecount)));
  printf( "Wopr ppl1:  %8.2f ", 10 ** (-$wopr_sumlog10/($wordcount-$oovcount)));
  print " (No oov words.)\n";
} else  {
  printf( "Wopr ppl:  %8.2f ", 10 ** (-$wopr_sumlog10/($wordcount+$sentencecount)));
  printf( "Wopr ppl1:  %8.2f ", 10 ** (-$wopr_sumlog10/($wordcount)));
  print " (With oov words.)\n";
}

close(FHW);

# NB, perl log is base e.
#
sub log2tolog10 {
  my $l    = shift;
  my $e    = 2.718281828;
  my $log2 = 0.3010299957;

  return $l * $log2;
}

sub log2 {
  my $l    = shift;

  if ( $l == 0 ) {
    return 0;
  }
  return log($l)/log(2);
}

sub log10 {
  my $l    = shift;

  if ( $l == 0 ) {
    return 0;
  }
  return log($l)/log(10);
}

__END__

ngt output:
# word prob n ngram
<s> 0.0403478 1 <s>
The 0.0985 2 <s> The
<unk> 0 0 OOV
