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
    for line in scf_lines:
        # but he had enough to learned (had) -14.0397 2.84644 16840.7 8 [ earned 1 ]
        # (.*)+ \(.*?\) (\d+) (\d+) (\d+) (\d+) \[ (.*?) (\d+) \]
        # ('let it not', 'pass', '-6.39197', '1.98777', '83.9797', '5', ' ')
        # lp, entropy, word_lp, dist_size 
        m = re.match( "(.*?) \((.*?)\) ([0-9\.,e+\-]+) ([0-9\.,e+\-]+) ([0-9\.,e+\-]+) ([0-9\.,e+\-]+) \[ (.*?)\]", line )
        if m:
            #print m.groups()
            line_count += 1
            logprob  = float(m.group(3))
            entropy  = float(m.group(4))
            wordlp   = float(m.group(5))
            distsize = int(m.group(6))
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
        else:
            if show_missed:
                print line[:-1] 

    print "File", scf
    print "Lines", line_count
    print "Logprob", min_logprob, max_logprob
    print "Entropy", min_entropy, max_entropy
    print "Wordlp", min_wordlp, max_wordlp
    print "Distsize", min_distsize, max_distsize
