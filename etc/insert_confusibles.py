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
Takes a plain text file, and inserts confusibles/errors

The error file looks like:
aan aam 0.1
...
"""

def levenshtein_wp(s1, s2):
    if len(s1) < len(s2):
        return levenshtein_wp(s2, s1)
    if not s1:
        return len(s2)
 
    previous_row = xrange(len(s2) + 1)
    for i, c1 in enumerate(s1):
        current_row = [i + 1]
        for j, c2 in enumerate(s2):
            insertions = previous_row[j + 1] + 1 # j+1 instead of j since previous_row and current_row are one character longer
            deletions = current_row[j] + 1       # than s2
            substitutions = previous_row[j] + (c1 != c2)
            current_row.append(min(insertions, deletions, substitutions))

        previous_row = current_row
 
    return previous_row[-1]

def damerau(seq1, seq2):
    oneago = None
    thisrow = range(1, len(seq2) + 1) + [0]
    for x in xrange(len(seq1)):
        # Python lists wrap around for negative indices, so put the
        # leftmost column at the *end* of the list. This matches with
        # the zero-indexed strings and saves extra calculation.
        twoago, oneago, thisrow = oneago, thisrow, [0] * len(seq2) + [x + 1]
        for y in xrange(len(seq2)):
            delcost = oneago[y] + 1
            addcost = thisrow[y - 1] + 1
            subcost = oneago[y - 1] + (seq1[x] != seq2[y])
            thisrow[y] = min(delcost, addcost, subcost)
            # This block deals with transpositions
            if (x > 0 and y > 0 and seq1[x] == seq2[y - 1]
                and seq1[x-1] == seq2[y] and seq1[x] != seq2[y]):
                thisrow[y] = min(thisrow[y], twoago[y - 2] + 1)
    return thisrow[len(seq2) - 1]

def add_error(w, e, p):
    if p == 0: #real 0 is ignore
        return
    ld = damerau(w, e)
    #print w, e, ld
    try:
        vk_ld_counts[ld] += 1
    except:
        vk_ld_counts[ld] = 1
    if ld > 0 and ld <= max_ld and len(w) >= min_l:
        n = int(p * 100); #assume percentages in data file (fill till 100 at end?)
        if n == 0:
            n = 1;
        try:
            #print e, n
            vk_errors[w].extend( [e] * n) 
        except:
            vk_errors[w] =  [e] * n 
    
afile  = None
vkfile = "ValkuilErrors.1.0_ns"
prob   = 10 # prob/10 that we make one change (if possible)
min_l  = 3  # minimum length of word >= min_l
max_ld = 10 # maximum LD <= max_ld, much higher because we supply errors from list
skip   = 0  # errors every Nth line, 0 & 1 is in all lines

try:
    opts, args = getopt.getopt(sys.argv[1:], "cd:f:l:m:v:p:s:", ["file="])
except getopt.GetoptError, err:
    print str(err)
    sys.exit(1)
for o, a in opts:
    if o in ("-f", "--file="):
        afile = a 
    elif o in ("-v"): 
        vkfile = a
    elif o in ("-c"): 
        confusibles = True
    elif o in ("-m"): 
        min_l = int(a)
    elif o in ("-d"): 
        max_ld = int(a)
    elif o in ("-p"): 
        prob = int(a)
    elif o in ("-s"): 
        skip = int(a)
    else:
        assert False, "unhandled option"

f_path, f_name = os.path.split(afile)
afile_outs = f_name+".cf" # with errors

vk_errors = {} # word => [ e1 e1 e2 e3 e3 e3 ]
#l.extend([x] * 100)
vk_ld_counts = {}
with open(vkfile, "r") as f:
    for line in f:
        if line == "" or line[0] == "#":
            continue
        line = line[:-1]
        # two words and prob
        m = re.match( "^([A-Za-z]*) ([A-Za-z]*) ([0\.1234656789]+)$", line)
        if m:
            w = m.group(1)
            e = m.group(2)
            p = float(m.group(3))
            add_error(w, e, p)
            continue
        # three words
        m = re.match( "^([A-Za-z]*) ([A-Za-z]*) ([A-Za-z]*)$", line)
        if m:
            w1 = m.group(1)
            w2 = m.group(2)
            w3 = m.group(3)
            p = 0.5 # ?
            add_error(w1, w2, p)
            add_error(w1, w3, p)
            add_error(w2, w3, p)
            add_error(w2, w1, p)
            add_error(w3, w1, p)
            add_error(w3, w2, p)
            continue
        # two words only
        m = re.match( "^([A-Za-z]*) ([A-Za-z]*)$", line)
        if m:
            w = m.group(1)
            e = m.group(2)
            p = 0.5 # ?
            add_error(w, e, p)
            add_error(e, w, p) #reverse them

            
'''
Normalise the error lists to 100 items (?)
'''
for err in vk_errors:
    #print err, len( vk_errors[err]), vk_errors[err]
    if len( vk_errors[err]) > 100:
        print "Hmm, more than 100."
    if  len( vk_errors[err] ) < 100:
        vk_errors[err].extend( [err] * (100-len( vk_errors[err])) )
        #print err, len( vk_errors[err]), vk_errors[err]

r = random.Random()
made_changes = 0
poss_changes = 0
changed = {}
lds = {}
skip_cnt = skip
with open(afile, "r") as f:
    with open(afile_outs, "w") as fo:
        for line in f:
            print line[:-1]
            skip_cnt -= 1
            if skip_cnt > 0:
                fo.write(line) #write unmodified line
                continue
            # Every skip-th line, we introduce errors.
            skip_cnt = skip
            line = line[:-1]
            words = line.split()
            indexes = [] #list of index->word which we can errorify
            idx = 0
            for word in words:
                if word in vk_errors:
                    indexes.append(idx)
                    poss_changes += 1
                    numerrs = len( vk_errors[word] )
                    randerr = int( r.uniform(0, numerrs) )
                    #print idx, word, vk_errors[word], vk_errors[word][randerr]
                idx += 1
            #print indexes
            #at these positions we can introduce an error
            num = int( r.uniform(0, 10) ) #create error if [0, 1] out of [0, ... 9] (1/5)
            numindexes = len(indexes)
            # create one or two errors per line
            randomidxs = []
            if num < prob and numindexes > 0 and numindexes < 3:
                randomidxs = [ indexes[ int( r.uniform(0, numindexes) ) ] ] #choose one,
            elif num < prob and numindexes > 0 and numindexes > 2:
                randomidxs = random.sample(set(indexes), 2) #take two random
            for randomidx in randomidxs:
                w = words[ randomidx ] #the word from the line
                numerrs = len( vk_errors[w] ) #number of possible errors
                randerr = int( r.uniform(0, numerrs) ) #choose one of them
                words[ randomidx ] = vk_errors[w][randerr]
                ld = damerau(w, vk_errors[w][randerr])
                if ld > 0: #could be the original word in the array
                    print "CHANGE", w, words[ randomidx ], ld
                    try:
                        changed[ vk_errors[w][randerr] ] += 1
                    except:
                        changed[ vk_errors[w][randerr] ] = 1
                    try:
                        lds[ld] += 1
                    except:
                        lds[ld] = 1
                    made_changes += 1
            new_line = ' '.join(words)
            fo.write(new_line+"\n")

print "Made", made_changes, "changes out of", poss_changes, "possible changes."


for ld in vk_ld_counts:
    print "VKLD", ld, vk_ld_counts[ld]

for change in changed:
    print "FREQ", change, changed[change]
for ld in lds:
    print "LD", ld, lds[ld]
