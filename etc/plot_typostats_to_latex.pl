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

use vars qw/ $opt_f $opt_n $opt_p $opt_r $opt_s $opt_t /;

getopts('f:n:pr:s:t');

my $data_file   = $opt_f || "";   #original data file, two lines (normal TOP1) 
my $cycle_nr    = $opt_n || "";   #cycle nr (like 14200)
my $percentages = $opt_p || 0;    #print percentages
my $re          = $opt_r || ".*"; #regular expression to match lines
my $stat_file   = $opt_s || "";   #the accompanying stats file
my $text_out    = $opt_t || 0;

if ( $cycle_nr ne "" ) {
  $data_file = "DATA.".$cycle_nr.".plot";
  $stat_file = "STAT.".$cycle_nr.".data";
}

#------------------------------------------------------------------------------

# TPO11003 100000 18370 350 181 333 402 162 51.71 95.14 114.86 46.29 l4r2_-a1+D hpx
# TPO11003 100000 18370 350 170 112 18 162 48.57 32.00 5.14 46.29 l4r2_-a1+D hpx  TOP1
#                                                              sometimes md5sum ^  
# Sometimes without                        ^^^^^^^^^^^^^^^^^^^^^^ these

# TPO13300 10000 l2r0_-a1+D 18370 1318 519 37939 241 757 d 760 757 37939 -20887 1.96 50.10 -109.56 3.78 c 519 998 37939 -20887 1.35 34.21 -110.88 2.60
# TPO13300 10000 l2r0_-a1+D 18370 1318 488 6210 73 757 d 561 757 6210 10842 8.29 42.56 62.07 13.87 c 488 830 6210 10842 7.29 37.03 61.68 12.18 TOP1
#                                                      ^detection           ^pre ^rec  ^acc  ^f1   ^correction

# indices to data line
my $gs = 4;
my $ws = 6;
my $ns = 7;
my $bs = 5;
my $algoffset = 3;

my $d_offset = 9; #pos of the 'd'
my $c_offset = 18; #pos of the 'c'

my $pre = $d_offset + 5;
my $rec = $d_offset + 6;
my $acc = $d_offset + 7;
my $f1s = $d_offset + 8;

my $prev_alg = "";

open(FHD, $data_file) || die "Can't open file.";
open(FHS, $stat_file) || die "Can't open file.";

# precision: PPV
# recall:    TPR
# accuracy:  ACC

# Values, ctx_alg as index.
my %b_ppv;
my %b_tpc;
my %b_acc;
my %b_fsc;

my %b_gsd;
my %b_gs;
my %b_ws;
my %b_ns;
my %b_bs;

if ( $text_out ) {
  print "ctx  alg     gsd    gs    ws    ns    bs   pre   rec   acc fscore\n";
  } else {
print << "EOF";
% plot_typostats_to_latex.pl -f $data_file -s $stat_file -r "$re"
\\begin{table}[h!]
\\caption{$data_file}
\\centering
\\vspace{\\baselineskip}
%\\fit{%
\\begin{tabular}{\@{} l r r r r r r r r r \@{}} %
\\toprule
 & \\gsd{} & \\gs{} & \\ws{} & \\ns{} & \\bs{} & \\pre{} & \\rec{} & \\acc{} & \\fscore{} \\\\
\\midrule
EOF
}

while ( my $ls0 = <FHD> ) {
  my $ls1 = <FHD>;

  my $s0 = <FHS>;  #two files are assumed to be // !!
  my $s1 = <FHS>;

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
      $alg = $l1[ $numl1 - ($algoffset+1) ];
    }
    #$alg =~ s/_\-a1\+D/ \\igtree{}/;
    #$alg =~ s/_\-a4\+D/ \\triblt{}/;

    my $sum_gswsns = $l1[$gs]+$l1[$ws]+$l1[$ns];

    # STAT file
    chomp $s0;
    my @sbits0 = split(/\ /, $s0);
    chomp $s1;
    my @sbits1 = split(/\ /, $s1);

    if ( $sbits1[0] ne $l1[0] ) {
      print "ERROR: ".$sbits1[0]."/".$l1[0]."\n";
      #print $ls0,"\n";
      #print $ls1,"\n";
      #print $s0,"\n";
      #print $s1,"\n";
    }
    my $total_errors = $sbits1[4];
    if ( $percentages ) {
      for ( my $i =4; $i < 8; $i++ ) {
        $l0[$i] = sprintf( "%5.2f", $l0[$i]*100/$total_errors);
        $l1[$i] = sprintf( "%5.2f", $l1[$i]*100/$total_errors);
      }
    }

	my $b_idx = $alg;
	$b_gs{$alg} = $l1[$gs];
	$b_ws{$alg} = $l1[$ws];
	$b_ns{$alg} = $l1[$ns];
	$b_bs{$alg} = $l1[$bs];

	$b_ppv{$alg} = $sbits1[$pre];
	$b_tpc{$alg} = $sbits1[$rec];
	$b_acc{$alg} = $sbits1[$acc];
	$b_fsc{$alg} = $sbits1[$f1s];

    if ( $text_out ) {
      my $out = $alg." ".&j($l0[$gs])." "; #gsd
      $out = $out . &j($l1[$gs])." ".&j($l1[$ws])." ".&j($l1[$ns])." ".&j($l1[$bs])." "; 
      $out = $out . &j($sbits1[$pre])." ".&j($sbits1[$rec])." ".&j($sbits1[$acc])." ".&j($sbits1[$f1s])." ";
      $out = $out . " ".$l1[0].",".$l1[1].",".$sbits1[4];
      print $out,"\n";
    } else {
      $alg =~ s/_\-a1\+D/ \\igtree{}/;
      $alg =~ s/_\-a4\+D/ \\triblt{}/;
	  my $alg_bit = substr $alg, -9;
	  my $ctx = substr $alg, 0, 4;
	  if ( $alg_bit ne $prev_alg ) {
		print "$alg_bit\\\\\n";
		$prev_alg = $alg_bit;
	  }
      my $out = "\\cmp{~".$ctx."} & \\num{".$l0[$gs]."} & \\num{";
      $out = $out . $l1[$gs]."} & \\num{".$l1[$ws]."} & \\num{".$l1[$ns]."} & \\num{".$l1[$bs]."} & \\num{"; 
      $out = $out . $sbits1[$pre]."} & \\num{".$sbits1[$rec]."} & \\num{".$sbits1[$acc]."} & \\num{".$sbits1[$f1s]."} ";
      $out = $out . "\\\\ %".$l1[0]." ".$l1[1]." ".$sbits1[4];
      print $out,"\n";
    }
  }
}
close(FHD);

# \\midrule
# \\igtree{}\\\\
if ( ! $text_out ) {
print << "EOT";
\\bottomrule
\\end{tabular}%}%endfit
\\label{tab:$data_file}
\\end{table}
EOT
}

# Bests
print "\n";
print "% ".&best( \%b_gs, "gs", "h" )."     ".&best( \%b_gs, "gs", "l" )."\n";
print "% ".&best( \%b_ws, "ws", "l" )."     ".&best( \%b_ws, "ws", "h" )."\n"; 
print "% ".&best( \%b_ns, "ns", "l" )."     ".&best( \%b_ns, "ns", "h" )."\n";
print "% ".&best( \%b_bs, "bs", "l" )."     ".&best( \%b_bs, "bs", "h" )."\n";

print "\n";
print "% ".&best( \%b_ppv, "pre", "h" )."     ".&best( \%b_ppv, "pre", "l" )."\n";
print "% ".&best( \%b_tpc, "rec", "h" )."     ".&best( \%b_tpc, "rec", "l" )."\n";
print "% ".&best( \%b_acc, "acc", "h" )."     ".&best( \%b_acc, "acc", "l" )."\n";
print "% ".&best( \%b_fsc, "fsc", "h" )."     ".&best( \%b_fsc, "fsc", "l" )."\n";


sub i {
    my $v = shift || 0; 
    return sprintf("%5d", $v)
}
sub f {
    my $v = shift || 0; 
	if ( $v < 100 ) {
	  return sprintf("%5.2f", $v)
	}
	if ( $v < 1000 ) {
	  return sprintf("%5.1f", $v)
	}
	return sprintf("%5.0f", $v)
}
sub j {
  my $v = shift || 0;
  if ( $v =~ m/\d+\.\d+/ ) {
	return &f($v);
  }
  return &i($v);
}
sub best{
  my $br = shift;
  my %b0 = %$br;
  my $nm = shift; #name
  my $hl = shift; #high low
  
  if ( $hl eq "h" ) {
	foreach (sort { ($b0{$b} <=> $b0{$a}) } keys %b0) {
	  return "best  $nm: $_ ".&j($b0{$_});
	  #last;
	}
  } else {
	foreach (sort { ($b0{$a} <=> $b0{$b}) } keys %b0) {
	  return "worst $nm: $_ ".&j($b0{$_});
	  #last;
	}
  }
}
__END__

