#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
import time, datetime
import os
import re
import sys
import datetime
import time
import getopt
import tempfile
from math import log

'''
Examines wopr .sc and .px output files and produces plot files for Gnuplot
to produce pretty graphs.

File format is grokked automatically, but it needs top-n [...] output.
'''

def find_bin(v):
    '''
    #{0: (1, 1), 1: (2, 2), 2: (3, 9), 3: (10, 99), 4: (100, 999), 5: (1000, 9999), 6: (10000, 99999)}
    Find bin for e.g 6: log(6)/log(10) = 0.77, 0+2 = 2: (3, 9)
    '''
    if v == 1:
        return 0
    if v == 2:
        return 1
    if v == 3:
        return 2
    idx = int(log(v)/log(pwr))
    return idx+1

sc_file     = None
all_files   = []
show_missed = False
x_range = "[]"
y_range = "[]"
normalized = False
topn = 0
binned = False

# Create bins, special for 1 and 2?
#0 1 1
#1 2 3
#2 4 7
#3 8 15
#4 16 31
#
# find bin for 6: log_{2}(6) = 2.58 int()=2, bin 4-7
pwr = 2
bins = {}
bin_counts = {}
# dustbin for left overs?
# Two special bins for 1 and 2.
bins[0] = (1, 1, "1") #inclusive
bin_counts[0] = 0
bins[1] = (2, 2, "2")
bin_counts[1] = 0
bin_idx = 2
s = 1
e = 17
for x in xrange(s,e+1): #for pwr==10 we need a better algo here and above here.
    #print x, pow(pwr,x), pow(pwr,x+1)-1
    if bin_idx == 2 and pwr == 2:
        bins[bin_idx] = (3, 3, "3")
    else:
        bins[bin_idx] = (pow(pwr,x), pow(pwr,x+1)-1, str(pow(pwr,x))+"-"+str(pow(pwr,x+1)-1))
    bin_counts[bin_idx] = 0
    bin_idx += 1
#print repr(bins) #{0: (1, 1), 1: (2, 2), 2: (3, 9), 3: (10, 99), 4: (100, 999), 5: (1000, 9999), 6: (10000, 99999)}
for x in range(1,12,1):
    print x, find_bin(x)
print 68900, find_bin(68900)

try:
    opts, args = getopt.getopt(sys.argv[1:], "bd:f:nst:x:y:", ["file="])
except getopt.GetoptError, err:
    #print str(err)
    print
    print "Examples"
    print "Process single file:"
    print "python examine_sc.py -s -f austen.test.l2r0_14546.sc"
    print "Process all files matching a regular expression:"
    print 'python examine_sc.py -d "nyt.*HPX66.*\.px$"'
    print
    sys.exit(2)
for o, a in opts:
    if o in ("-f", "--file"):
        all_files = [ a ]
    elif o in ("-b", "--binned"):
        binned = True
    elif o in ("-d", "--dir"): # -d "nyt.*HPX66.*\.px$"
        test  = re.compile(a, re.IGNORECASE)
        files = os.listdir( "." )
        files = filter(test.search, files)    
        for a_file in files:
            all_files.append(a_file)
    elif o in ("-n", "--normalized"):
        normalized = True
        y_range = "[0:1]"
    elif o in ("-x", "--x-range"):
        x_range = a
    elif o in ("-y", "--y-range"):
        y_range = a
    elif o in ("-t", "--topn"):
        topn = int(a)
    else:
        assert False, "unhandled option"

#print all_files
for scf in all_files:
    scf_lines = []
    with open(scf, 'r') as f:
        scf_lines = f.readlines()
    lc = 0
    rc = 0
    m = re.match( ".*l(\d+)r(\d+).*", scf)
    if m:
        lc = int(m.group(1))
        rc = int(m.group(2))
    hpx = 0
    m = re.match( ".*hpx(\d+).*", scf)
    if m:
        hpx = int(m.group(1))
    sum_logprob  =     0.0
    min_logprob  =     0.0
    max_logprob  =   -99.0
    min_distsize =  1000
    max_distsize =     0
    max_distfreq =     0 # size of distribution which occurs most
    min_entropy  =     0.0
    max_entropy  =   -99.0
    line_count   =     0
    min_wordlp   =  1000.0
    max_wordlp   =     0.0

    distr_distsize = {}

    for line in scf_lines:
        # but he had enough to learned (had) -14.0397 2.84644 16840.7 8 [ earned 1 ]
        # (.*)+ \(.*?\) (\d+) (\d+) (\d+) (\d+) \[ (.*?) (\d+) \]
        # ('let it not', 'pass', '-6.39197', '1.98777', '83.9797', '5', ' ')
        # lp, entropy, word_lp, dist_size
        # 
        logprob  = 0
        entropy  = 0
        wordlp   = 0
        distsize = 0
        matched  = False
        # Match for *.sc files
        m = re.match( "(.*?) \((.*?)\) ([0-9\.,e+\-]+) ([0-9\.,e+\-]+) ([0-9\.,e+\-]+) ([0-9\.,e+\-]+) \[ (.*?)\]", line )
        if m:
            #print m.groups()
            line_count += 1
            logprob  = float(m.group(3))
            entropy  = float(m.group(4))
            wordlp   = float(m.group(5))
            distsize = int(m.group(6))
            matched  = True
        else:
            # Match for *.px files
            # instance+target classification log2prob entropy word_lp guess k/u md mal dist.cnt dist.sum RR ([topn])
            # _ _ , the Instead Still -4.82762 7.70098 28.396 cd k 3 0 1056 5736 0.2 [ Still 412 Meanwhile 297 However 273 Indeed 223 Instead 202 ]
            m = re.match( "(.*?) ([0-9\.,e+\-]+) ([0-9\.,e+\-]+) ([0-9\.,e+\-]+) (cd|cg|ic) (k|u) ([0-9\.,e+\-]+) ([0-9\.,e+\-]+) ([0-9\.,e+\-]+) ([0-9\.,e+\-]+) ([0-9\.,e+\-]+)( \[ (.*?)\])?", line)
            if m:
                line_count += 1
                logprob  = float(m.group(2))
                entropy  = float(m.group(3))
                wordlp   = float(m.group(4))
                distsize = int(m.group(9))
                matched  = True
        if matched:
            sum_logprob += logprob
            if logprob < min_logprob:
                min_logprob = logprob
            elif logprob > max_logprob:
                max_logprob = logprob
            if entropy < min_entropy:
                min_entropy = entropy
            elif entropy > max_entropy:
                max_entropy = entropy
            if distsize < min_distsize:
                min_distsize = distsize
            elif distsize > max_distsize:
                max_distsize = distsize
            if wordlp < min_wordlp:
                min_wordlp = wordlp
            elif wordlp > max_wordlp:
                max_wordlp = wordlp

            try:
                distr_distsize[distsize] += 1
            except:
                distr_distsize[distsize] = 1
            if binned:
                bin = find_bin(distsize)
                try:
                    bin_counts[bin] += 1
                except:
                    print "No bin for", bin, distsize

        if not matched and show_missed:
            print line[:-1] 

    print "File",     scf
    print "Lines",    line_count
    print "Logprob",  min_logprob, max_logprob
    print "Entropy",  min_entropy, max_entropy
    print "Wordlp",   min_wordlp, max_wordlp
    print "Distsize", min_distsize, max_distsize

    # Find dist freq which occurs most
    distrsizes_sum = 0
    for distrsize, distrsize_count in iter(sorted(distr_distsize.iteritems())):
        distrsizes_sum += distrsize_count
        if distrsize_count > max_distfreq:
            max_distfreq = distrsize_count

    # File with data, and normalized distr size.
    # Note that using topn truncates the data file!
    # Indexed top-n? That is print distr_distsize[cnt]
    scfd = scf + ".ds.data"
    print "Data file", scfd
    with open(scfd, 'w') as f:
        cnt = topn # cnt = topn if topn > 0 else 0
        for distrsize, distrsize_count in iter(sorted(distr_distsize.iteritems())):
            f.write(str(distrsize)+" "+str(distrsize_count)+" "+str(float(distrsize_count)/float(max_distfreq))+" "+str(float(distrsize_count)/float(line_count))+"\n")
            cnt -= 1
            if cnt == 0:
                break

    # Indexed top-n, that is print distr_distsize[cnt]
    # Or, an absolute top-1, print "0 0.00" for non existing
    # sizes.
    scfd = scf + ".ds.data2"
    print "Data file", scfd
    with open(scfd, 'w') as f:
        for cnt in xrange(1,topn+1 if topn > 0 else max_distsize+1):
            distrsize = cnt
            try:
                distrsize_count = distr_distsize[cnt]
                f.write(str(distrsize)+" "+str(distrsize_count)+" "+str(float(distrsize_count)/float(max_distfreq))+" "+str(float(distrsize_count)/float(line_count))+"\n")
            except KeyError:
                f.write(str(distrsize)+" 0 0.00\n")

    if binned:
        scfdb = scf + ".ds.bins"
        print "Data file", scfdb
        with open(scfdb, 'w') as f:
            for x in xrange(0,e+1):
                f.write(str(x)+" "+str(bin_counts[x])+" "+str(pwr)+" "+str(bins[x][0])+" "+str(bins[x][1])+" "+str(bins[x][2])+"\n")
                if bins[x][1] > max_distsize:
                    pass
                    #break #to stop after largest

    # Gnuplot file
    scfp = scf + ".ds.plot"
    #http://xmodulo.com/2013/01/how-to-plot-data-without-data-files-in-gnuplot.html
    with open(scfp, 'w') as f:
        f.write("# Created by examine_sc.py -f "+scf+"\n" )
        f.write("set style line 1 lc rgb '#0060ad' lt 1 lw 1\n")
        info_str = "l"+str(lc)+"r"+str(rc)
        if hpx > 0:
            info_str += ", hapax "+str(hpx)
        f.write("set title \"Distribution Size, "+info_str+"\"\n")
        f.write("set size 1.3,1\n")
        f.write("set xlabel \"Size of distribution\"\n")
        f.write("set xrange "+x_range+"\n")
        f.write("set xtics out\n")
        f.write("set xtics rotate by 295 nomirror\n")
        f.write("set rmargin 5\n")
        f.write("set ytics out nomirror\n")
        f.write("set border 3\n") #http://ontublog.blogspot.se/2009/09/complex-axis-in-gnuplot.html
        f.write("set yrange "+y_range+"\n")
        f.write("set style data histogram\n")
        f.write("set style histogram cluster gap 1\n")
        f.write("set style fill solid\n") # (...solid border -1), and set boxwidth 0.8
        if normalized:
            f.write("set ylabel \"normalized frequency, 1.00 = "+str(max_distfreq)+"\"\n")
            f.write("set ytics 0.1\n")
        else:
            f.write("set ylabel \"frequency of size\"\n")
        if normalized:
            f.write("plot '"+scfd+"' using 3:xticlabels(1) ls 1 title \"\"\n")
        else:
            f.write("plot '"+scfd+"' using 2:xticlabels(1) ls 1 title \"\"\n")
        f.write("set terminal push\n")
        f.write("set terminal postscript eps enhanced rounded lw 2 'Helvetica' 20\n")
        f.write("set out '"+scf+".ds.ps'\n")
        f.write("replot\n")
        f.write("!epstopdf '"+scf+".ds.ps'\n")
        f.write("set term pop\n")
        #f.write("pause -1\n")
    print "Gnuplot file", scfp

    # Gnuplot file if binned
    if binned:
        scfp = scf + ".ds.bins.plot"
        with open(scfp, 'w') as f:
            f.write("# Created by examine_sc.py -f "+scf+"-b \n" )
            f.write("set style line 1 lc rgb '#0060ad' lt 1 lw 1\n")
            info_str = "l"+str(lc)+"r"+str(rc)
            if hpx > 0:
                info_str += ", hapax "+str(hpx)
            f.write("set title \"Distribution Size, "+info_str+"\"\n")
            f.write("set size 1.3,1\n")
            f.write("set xlabel \"Size of distribution\"\n")
            f.write("set xrange "+x_range+"\n")
            f.write("set xtics out\n")
            f.write("set xtics rotate by 295 nomirror\n")
            f.write("set xtics font \"Helvetica,14\"\n")
            #f.write("set label font \"Helvetica,12\"\n")
            f.write("set rmargin 5\n")
            f.write("set ytics out nomirror\n")
            f.write("set border 3\n") #http://ontublog.blogspot.se/2009/09/complex-axis-in-gnuplot.html
            f.write("set yrange "+y_range+"\n")
            f.write("set style data histogram\n")
            f.write("set style histogram cluster gap 1\n")
            f.write("set style fill solid\n") # (...solid border -1), and set boxwidth 0.8
            if normalized:
                f.write("set ylabel \"normalized frequency, 1.00 = "+str(max_distfreq)+"\"\n")
                f.write("set ytics 0.1\n")
            else:
                f.write("set ylabel \"frequency of size\"\n")
            if normalized:
                f.write("plot '"+scfdb+"' using 3:xticlabels(6) ls 1 title \"\"\n")
            else:
                f.write("plot '"+scfdb+"' using 2:xticlabels(6) ls 1 title \"\"\n") #,'' using 0:($2+500):2 with labels title \"\"
            # "hand drawn" labels on top of bar is low frequency
            for x in xrange(0,e+1):
                if bin_counts[x] < 100:
                    f.write("set label \"{/Helvetica=14 "+str(bin_counts[x])+"}\" at "+str(x-0.25)+","+str(bin_counts[x]+500)+"\n")
            f.write("set terminal push\n")
            f.write("set terminal postscript eps enhanced rounded lw 2 'Helvetica' 20\n")
            f.write("set label font \"Helvetica,12\"\n")
            f.write("set out '"+scf+".ds.bins.ps'\n")
            f.write("replot\n")
            f.write("!epstopdf '"+scf+".ds.bins.ps'\n")
            f.write("set term pop\n")
            #f.write("pause -1\n")
        print "Gnuplot file", scfp

'''
set xtics ("332" 332, "336" 336, "340" 340, "394" 394, "398" 398, "1036" 1036)

plot 'data.dat' using ($1-0.5):2 ti col smooth frequency with boxes
, '' u ($1-0.25):3 ti col smooth frequency with boxes, '' u ($1+0.25):4 ti col smooth frequency with boxes, '' u ($1+0.5):5 ti col smooth frequency with boxes

set terminal pngcairo enhanced font "arial,10" size 1440, 900
set output 'tmp_in_48hr.png'
set grid xtics ytics
set xtics 7200 rotate by -60
set rmargin at screen 0.95
set ytics 1
set key below
unset colorbox
set palette rgbformulae 33,13,10
plot [][15:25] "tmp_in_48hr.data" using 3:2:2 title "temperatuur" with lines palette lw 2

xtics_str = ""
for distrsize, distrize_count in iter(sorted(distr_distsize.iteritems())):
    xtics_str += str(distrsize)+','
xtics_str = xtics_str[:-1] #remove last ,
f.write("set xtics ("+xtics_str+")\n")
f.write("plot '-' using ($1):2 ti col smooth frequency with boxes\n")

'''

