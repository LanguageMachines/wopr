
#!/usr/bin/python
# -*- coding: utf-8 -*-
#
import getopt
import sys
import time
import os
import datetime
import random
import re
import platform
from operator import itemgetter
import operator

"""
Takes a plain text file, and inserts errors from the Valkuil error file.

The error file looksl ike:
aan~aam
aan~aans
aan~ana
aanbesteding~aanbsteding
aanbestedingsprocedure~aanstedingsprocedure
aanbestedingsreglement~aanbestedingsregelement
aanbestedingsresultaten~aanbestedingsresulaten
aanbevolen~aangebevolen
aanbieding~aambieding
aanboden~aanbooden

Input is plain text files, one sentence per line.
"""

afile = None
vkfile = "ValkuilErrors.1.0_ns"

try:
    opts, args = getopt.getopt(sys.argv[1:], "f:v:", ["file="])
except getopt.GetoptError, err:
    print str(err)
    sys.exit(1)
for o, a in opts:
    if o in ("-f", "--file="):
        afile = a 
    elif o in ("-v"): 
        vkfile = a
    else:
        assert False, "unhandled option"

f_path, f_name = os.path.split(afile)
afile_outs = f_name+".vkerr" # with spelling errors

vk_errors = {} # word => [ er1, er2 ]
with open(vkfile, "r") as f:
    for line in f:
        line = line[:-1]
        #print line
        #aandacht~aantdacht
        #print line.split('~')
        m = re.match( "^([A-Za-z]*)~(.*)$", line)
        if m:
            w = m.group(1)
            e = m.group(2)
            #print w, e
            try:
                vk_errors[w].append(e) 
            except:
                vk_errors[w] = [ e ]

r = random.Random()
made_changes = 0
poss_changes = 0
with open(afile, "r") as f:
    with open(afile_outs, "w") as fo:
        for line in f:
            line = line[:-1]
            print line
            words = line.split()
            indexes = []
            idx = 0
            for word in words:
                if word in vk_errors:
                    indexes.append(idx)
                    poss_changes += 1
                    numerrs = len(vk_errors[word])
                    randerr = int( r.uniform(0, numerrs) )
                    #print idx, word, vk_errors[word], vk_errors[word][randerr]
                idx += 1
            #print indexes #at these positions we can introduce an error
            num = int( r.uniform(0, 10) ) #create error if [0, 1] out of [0, ... 9] (1/5)
            numindexes = len(indexes)
            # create one error per line
            if num < 2 and numindexes > 0:
                randomidx = int( r.uniform(0, numindexes) ) #choose one
                w = words[ indexes[randomidx] ] #the word from the line
                numerrs = len(vk_errors[w]) #number the possible errors
                randerr = int( r.uniform(0, numerrs) ) #choose one of them
                words[ indexes[randomidx] ] = vk_errors[w][randerr]
                print "CHANGE", w, words[ indexes[randomidx] ] 
                made_changes += 1
            new_line = ' '.join(words)
            fo.write(new_line+"\n")
print "Made", made_changes, "changes out of", poss_changes, "posible changes."

