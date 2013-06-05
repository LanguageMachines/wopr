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
        # 
        logprob  = 0
        entropy  = 0
        wordlp   = 0
        distsize = 0
        matched = False
        m = re.match( "(.*?) \((.*?)\) ([0-9\.,e+\-]+) ([0-9\.,e+\-]+) ([0-9\.,e+\-]+) ([0-9\.,e+\-]+) \[ (.*?)\]", line )
        if m:
            #print m.groups()
            line_count += 1
            logprob  = float(m.group(3))
            entropy  = float(m.group(4))
            wordlp   = float(m.group(5))
            distsize = int(m.group(6))
            matched = True
        else:
            # instance+target classification log2prob entropy word_lp guess k/u md mal dist.cnt dist.sum RR ([topn])
            # _ _ , the Instead Still -4.82762 7.70098 28.396 cd k 3 0 1056 5736 0.2 [ Still 412 Meanwhile 297 However 273 Indeed 223 Instead 202 ]
            m = re.match( "(.*?) ([0-9\.,e+\-]+) ([0-9\.,e+\-]+) ([0-9\.,e+\-]+) (cd|cg|ic) (k|u) ([0-9\.,e+\-]+) ([0-9\.,e+\-]+) ([0-9\.,e+\-]+) ([0-9\.,e+\-]+) ([0-9\.,e+\-]+)( \[ (.*?)\])?", line)
            if m:
                line_count += 1
                logprob  = float(m.group(2))
                entropy  = float(m.group(3))
                wordlp   = float(m.group(4))
                distsize = int(m.group(9))
                matched = True
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
            
            if not matched and show_missed:
                print line[:-1] 

    print "File", scf
    print "Lines", line_count
    print "Logprob", min_logprob, max_logprob
    print "Entropy", min_entropy, max_entropy
    print "Wordlp", min_wordlp, max_wordlp
    print "Distsize", min_distsize, max_distsize
