#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
import glob
import os
import re
import sys
import codecs
import getopt
import logging
logging.basicConfig(level=logging.INFO, #DEBUG
    format='%(asctime)s %(name)s: %(levelname)-8s %(message)s',
    datefmt='%H:%M:%S'
    #filename='./hoo2013.log', #filemode='w', datefmt='%Y-%m-%d %H:%M:%S',
)
console = logging.StreamHandler()
console.setLevel(logging.INFO)
gl = logging.getLogger('ROC') #global logger

"""
Select several words in different .roc files, create gnuplot
file.

Expect output and filenames generated by do_roc_nnnnn.sh

All words:
python roc_sel_plot.py -x 0.01

Selected words (the, have and has):
python roc_sel_plot.py -w the_have_has -x 0.01 -y 0.5

Choose plot range:
python roc_sel_plot.py  -y 0.5 --xr="[0:0.1]"

#accuracy over training lines:
plot [][] "< grep l2r0 in.roc" using 13:11
"""

# ----------------------------------------------------------------------------

class Segment():
    def __init__(self, word):
        self.word = word
        self.points = []
    def add_point(self, pt):
        self.points.append(pt)
    def to_gnuplot(self):
        for pt in self.points:
            print self.word, pt[0], pt[1]

# ----------------------------------------------------------------------------

"""
#TRAIN:/scratch/pberck/2013/data/nyt.3e7.1000000
#TEST:nyt.t10000
#PARAMS:l2r0_-a1+D
# word TP FP FN TN pt0 pt1 accuracy precision recall Fscore
At 0 0 74 228828 0.0 0.0 0.999676717547 -1 0.0 0.0
maturity 0 0 6 228896 0.0 0.0 0.999973787909 -1 0.0 0.0
"""

tr_str = "^#TRAIN:.*\.(\d+)"
re_tr = re.compile(tr_str, re.IGNORECASE)
te_str = "^#TEST:(.*)"
re_te = re.compile(te_str, re.IGNORECASE)
pa_str = "^#PARAMS:(.*)"
re_pa = re.compile(pa_str, re.IGNORECASE)

def process_roc(rocfile, word, x_min, y_min, of):
    gl.info("Reading file: "+rocfile)
    f = codecs.open(rocfile, "r", "utf-8")

    line = f.readline()
    m = re_tr.match(line)
    if m:
        tr_info = m.group(1) #number of lines
        gl.info(tr_info)
    else:
        gl.error("File contains no header")
        return
    line = f.readline()
    m = re_te.match(line)
    if m:
        te_info = m.group(1)
        gl.info(te_info)
    else:
        gl.error("File contains no header")
        return
    line = f.readline()
    m = re_pa.match(line)
    if m:
        pa_info = m.group(1)
        gl.info(pa_info)
    else:
        gl.error("File contains no header")
        return

    for line in f:
        if line[0] == "#":
            continue
        bits = line.split()
        if bits[0] == word or word == None:
            if float(bits[5]) > x_min and float(bits[6]) > y_min and float(bits[10]) > fscore_min:
                gl.info(line[:-1])
                of.write(line[:-1]+" "+pa_info+" "+tr_info+" "+rocfile+"\n")
                seg_word = bits[0] #works for both supplied word or None
                if seg_word not in segments.keys():
                    gl.info("New Segment("+seg_word+")")
                    segments[seg_word] = Segment(seg_word)
                segments[seg_word].add_point( (float(bits[5]),float(bits[6]),rocfile,float(bits[7]),float(bits[8]),float(bits[9]),float(bits[10])) )
    f.close()

    gl.info("Ready")

# ----------------------------------------------------------------------------

word = None
roc_str = "(.*)roc$"
out = "out"
x_min = 0
y_min = 0
fscore_min = 0
segments = dict()
x_rng = "[0:1]"
y_rng = "[0:1]"

try:
    opts, args = getopt.getopt(sys.argv[1:], "f:o:w:r:x:y:", ["xr=","yr=","word="])
except getopt.GetoptError, err:
    # print help information and exit:
    gl.error(str(err))
    sys.exit(2)
for o, a in opts:
    if o in ("-w"):
        word = a
    elif o in ("-r"):
        roc_str = a
    elif o in ("-r"):
        rc = int(a)
    elif o in ("-f"):
        fscore_min = float(a)
    elif o in ("-x"):
        x_min = float(a)
    elif o in ("-y"):
        y_min = float(a)
    elif o in ("--xr"):
        x_rng = a
    elif o in ("--yr"):
        y_rng = a
    else:
        gl.error("Unhandled option")
        assert False, "unhandled option"

roc_re  = re.compile(roc_str, re.IGNORECASE)
roc_files = os.listdir( "." )
roc_files = filter(roc_re.search, roc_files)

out_gnuplot_name  = out + ".gnuplot"
out_data_name     = out + ".roc.sel.data"
out_segments_name = out + ".roc.sel.segments"
out_labels_name   = out + ".roc.sel.labels"
gl.info("Output: "+out_gnuplot_name+"/"+out_data_name+"/"+out_segments_name+"/"+out_labels_name)

out_data_file = codecs.open(out_data_name, "w", "utf-8")

# Same format as .roc files, but little extra at end
# Same gnuplot file should work.

out_data_file.write("# word TP FP FN TN pt0 pt1 accuracy precision recall Fscore params trlines rocfile\n")

# Test if word is w0_w1 etc
if word:
    words = word.split('_')
    word_str = word.replace('_',',')
else:
    words = [ None ]
    word_str = "all words"
files_str = ""
for w in words:
    for roc_file in roc_files:
        gl.info(roc_file)
        process_roc(roc_file, w, x_min, y_min, out_data_file)
        files_str += roc_file+"\n# "
out_data_file.close()

out_segments_file = open(out_segments_name, 'w')
out_labels_file = open(out_labels_name, 'w')
#plot [0:1][0:1]"out.roc.sel.segments" using 2:3 with lines,\
# "out.roc.sel.labels" using 2:3:1 with labels
# Fscores:
# plot "out.roc.sel.segments" using ($0):8:xtic(1) with lines
# ($0) is line number, xtic(1) takes 1 as x label (short for xticlabels(1))
# (plot 'datafile' u 1:2:( xticlabels( stringcolumn(1).stringcolumn(2) ) ) )
#set xtics border rotate by 90 offset 0,-2
#set xtics border rotate by 270 offset 0,0 is better

# word pt0 pt1 rocfile accuracy precision recall Fscore
for sw in segments:
    s = segments[sw]
    out_labels_file.write(s.word) #write only the first pt in labels file, the ROC label will be printed at pt
    for i in xrange(0,7):
        out_labels_file.write(" "+str(s.points[0][i]))
    out_labels_file.write("\n")
    for pt in s.points:
        out_segments_file.write(s.word)
        for i in xrange(0,7):
            out_segments_file.write(" "+str(pt[i]))
        out_segments_file.write("\n")
    out_segments_file.write("\n") #blank line between to start new line segment
out_labels_file.close()
out_segments_file.close()

out_gnuplot_file = open(out_gnuplot_name, 'w')
files_str = files_str[:-3]
out_gnuplot_file.write("set title \"ROC on "+word_str+"\"\n")
out_gnuplot_file.write("# "+files_str+"\n")
out_gnuplot_file.write("set xlabel \"FPR\"\n")
out_gnuplot_file.write("set key bottom\n")
out_gnuplot_file.write("set ylabel \"TPR\"\n")
out_gnuplot_file.write("set grid\n")
out_gnuplot_file.write("plot "+x_rng+y_rng+" \""+out_data_name+"\" using 6:7 with points,\\\n")
out_gnuplot_file.write("\""+out_data_name+"\" using 6:7:1 with labels offset 1.5,0.5 notitle\n")
out_gnuplot_file.write("set terminal push\n")
out_gnuplot_file.write("set terminal postscript eps enhanced color solid rounded lw 2 'Helvetica' 10\n")
out_gnuplot_file.write("set out \""+out_data_name+".ps\"\n")
out_gnuplot_file.write("replot\n")
out_gnuplot_file.write("!epstopdf \""+out_data_name+".ps\"\n")
out_gnuplot_file.write("set term pop\n")
out_gnuplot_file.write("# plot with segments\n")
out_gnuplot_file.write("plot "+x_rng+y_rng+" \""+out_segments_name+"\" using 2:3 with lines,\\\n")
out_gnuplot_file.write("\""+out_labels_name+"\" using 2:3:1 with labels\n")
out_gnuplot_file.write("set terminal push\n")
out_gnuplot_file.write("set terminal postscript eps enhanced color solid rounded lw 2 'Helvetica' 10\n")
out_gnuplot_file.write("set out \""+out_segments_name+".ps\"\n")
out_gnuplot_file.write("replot\n")
out_gnuplot_file.write("!epstopdf \""+out_segments_name+".ps\"\n")
out_gnuplot_file.write("set term pop\n")
#set xtics border rotate by 270 offset 0,0
#plot "out.roc.sel.segments" using ($0):8:xtic(1) with lines
#
#set xtics border rotate by 270 offset 0,0
#set boxwidth 0.5
#set style fill solid
#plot "out.roc.sel.labels" using ($0):8:xtic(1) with boxes
#
# Make the x axis labels easier to read.
#set xtics rotate out
# Select histogram data
#set style data histogram
# Give the bars a plain fill pattern, and draw a solid line around them.
#set style fill solid border
#set style histogram clustered
#plot for [COL=2:4] 'date_mins.tsv' using COL:xticlabels(1) title columnheader
#
out_gnuplot_file.write("# histogram\n")
out_gnuplot_file.write("reset\n")
out_gnuplot_file.write("A = \"#99ffff\"; B = \"#4671d5\"; C = \"#ff0000\"; D = \"#f36e00\"\n")
out_gnuplot_file.write("set style data histogram\n")
out_gnuplot_file.write("set style histogram cluster gap 1\n")
out_gnuplot_file.write("set style fill solid border -1\n")
out_gnuplot_file.write("set xtics border rotate by 270 offset 0,0\n")
out_gnuplot_file.write("set boxwidth 0.9\n")
out_gnuplot_file.write("plot [][0:1] \\\n")
out_gnuplot_file.write(" \""+out_labels_name+"\" using 5:xtic(1) ti 'accuracy'\\\n")
out_gnuplot_file.write(",\""+out_labels_name+"\" using 6 ti \"precision\"\\\n")
out_gnuplot_file.write(",\""+out_labels_name+"\" using 7 ti \"recall\"\\\n")
out_gnuplot_file.write(",\""+out_labels_name+"\" using 8 ti \"Fscore\"\n")
out_gnuplot_file.write("set terminal push\n")
out_gnuplot_file.write("set terminal postscript eps enhanced color solid rounded lw 2 'Helvetica' 10\n")
out_gnuplot_file.write("set out \""+out_labels_name+".hist.ps\"\n")
out_gnuplot_file.write("replot\n")
out_gnuplot_file.write("!epstopdf \""+out_labels_name+".hist.ps\"\n")
out_gnuplot_file.write("set term pop\n")
#
out_gnuplot_file.close()

'''
plot [0:.005][0:1] "nyt.t10000.l2r0_ROC30035.px.roc" using 6:7:1 with labels
set terminal push
set terminal postscript eps enhanced color solid rounded lw 2 'Helvetica' 10
set out 'plot2f.ps'
replot
!epstopdf 'plot2f.ps'
set term pop

import subprocess
proc = subprocess.Popen(['gnuplot','-p'], 
                        shell=True,
                        stdin=subprocess.PIPE,
                        )
proc.stdin.write('set xrange [0:10]; set yrange [-2:2]\n')
proc.stdin.write('plot sin(x)\n')
proc.stdin.write('quit\n') #close the gnuplot window

proc.communicate("""
set xrange [0:10]; set yrange [-2:2]
plot sin(x)
pause 4
""")
'''
