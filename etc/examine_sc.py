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

sc_file = None
all_files = []
show_missed = False

try:
    opts, args = getopt.getopt(sys.argv[1:], "d:f:s", ["file="])
except getopt.GetoptError, err:
    print str(err)
    sys.exit(2)
for o, a in opts:
    if o in ("-f", "--file"):
        all_files = [ a ]
    elif o in ("-d", "--dir"):
        test  = re.compile(a, re.IGNORECASE)
        files = os.listdir( "." )
        files = filter(test.search, files)    
        for a_file in files:
            all_files.append(a_file)
    elif o in ("-s", "--show-missed"):
        show_missed = True
    else:
        assert False, "unhandled option"

print all_files
for scf in all_files:
    scf_lines = []
    with open(scf, 'r') as f:
        scf_lines = f.readlines()
    sum_logprob  =     0.0
    min_logprob  =     0.0
    max_logprob  =   -99.0
    min_distsize =  1000
    max_distsize =     0
    min_entropy  =     0.0
    max_entropy  =   -99.0
    line_count   =     0
    min_wordlp   =  1000
    max_wordlp   =     0

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

        if not matched and show_missed:
            print line[:-1] 

    print "File", scf
    print "Lines", line_count
    print "Logprob", min_logprob, max_logprob
    print "Entropy", min_entropy, max_entropy
    print "Wordlp", min_wordlp, max_wordlp
    print "Distsize", min_distsize, max_distsize

    # File with data, and normalized distr size.
    scfd = scf + ".ds.data"
    with open(scfd, 'w') as f:
        for distrsize, distrize_count in iter(sorted(distr_distsize.iteritems())):
            f.write(str(distrsize)+" "+str(distrize_count)+" "+str(distrsize*100.0/max_distsize)+"\n")

    # Gnuplot file
    scfp = scf + ".ds.plot"
    #http://xmodulo.com/2013/01/how-to-plot-data-without-data-files-in-gnuplot.html
    with open(scfp, 'w') as f:
        f.write("# Created by examine_sc.py -f "+scf+"\n" )
        f.write("set style line 1 lc rgb '#0060ad' lt 1 lw 5 pt 7 pi -1 ps 1.5\n")
        f.write("set title \"Distribution Size\"\n")
        f.write("set xlabel \"Size\"\n")
        f.write("set xrange []\n")
        f.write("set xtics out\n")
        f.write("set xtics rotate by 270\n")
        f.write("set ytics out\n")
        f.write("set yrange []\n")
        f.write("set ylabel \"number\"\n")
        f.write("plot '"+scfd+"' using 1:2 with impulses ls 1 title \"\"\n")
        f.write("set terminal push\n")
        f.write("set terminal postscript eps enhanced monochrome rounded lw 2 'Helvetica' 20\n")
        f.write("set out '"+scf+".ds.ps'\n")
        f.write("replot\n")
        f.write("!epstopdf '"+scf+".ds.ps'\n")
        f.write("set term pop\n")
        #f.write("pause -1\n")

'''
set xtics ("332" 332, "336" 336, "340" 340, "394" 394, "398" 398, "1036" 1036)

plot 'data.dat' using ($1-0.5):2 ti col smooth frequency with boxes
, '' u ($1-0.25):3 ti col smooth frequency with boxes, '' u ($1+0.25):4 ti col smooth frequency with boxes, '' u ($1+0.5):5 ti col smooth frequency with boxes

set terminal pngcairo enhanced font "arial,10" size 1440, 900
set output 'tmp_in_48hr.png'
set xdata time
set timefmt "%Y-%m-%dT%H:%M:%S"
set format x "%m-%d %H:%M"
set grid xtics ytics
set xtics 7200 rotate by -60
set rmargin at screen 0.95
set ytics 1
set title "Temperatuur binnen 48t"
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
