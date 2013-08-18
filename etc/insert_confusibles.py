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
try:
    from collections import Counter
except:
    pass

"""
Takes a plain text file, and inserts confusibles/errors

The error file looks like:
hun hen 0.3
de het                    (assign default_p)
in op onder naast 0.1

python insert_confusibles.py -v conf_nl.txt -f zin
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
    #print w, e, p, ld
    try:
        vk_ld_counts[ld] += 1
    except:
        vk_ld_counts[ld] = 1
    if ld > 0 and ld <= max_ld and len(w) >= min_l:
        n = int(p * 100); #assume percentages in data file (fill till 100 at end?)
        np = 100 - n #n'
        try:
            print w, e, n
            vk_errors[w].extend( [e] * n) 
            #vk_errors[w].extend( [w] * np) 
        except:
            vk_errors[w] = [e] * n 
            vk_errors[w].extend( [w] * np )

def add_error(w, e, p, s): 
    if p == 0: #real 0 is ignore
        return
    ld = damerau(w, e)
    #print w, e, p, ld
    try:
        vk_ld_counts[ld] += 1
    except:
        vk_ld_counts[ld] = 1
    if ld > 0 and ld <= max_ld and len(w) >= min_l:
        n = int(p * 100); #assume percentages in data file (fill till 100 at end?)
        np = 100 - (n*(s-1)) #n', compensate so total for sets > 2items is 100
        try:
            vk_errors[w].extend( [e] * n) 
            #vk_errors[w].extend( [w] * np) 
        except:
            vk_errors[w] = [e] * n 
            vk_errors[w].extend( [w] * np )

afile     =  None
vkfile    =  "ValkuilErrors.1.0_ns"
min_l     =  1   # minimum length of word >= min_l
max_ld    = 10   # maximum LD <= max_ld, much higher because we supply errors from list
skip      =  0   # errors every Nth line, 0 & 1 is in all lines
default_p =  0.5 #chance we take another from the confusible set

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
        default_p = float(a)
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
        words = line.split()
        p = default_p #chance is default_p that we take one other from the set # 1.0/len(words) 
        if re.match( "([0\.1234656789]+)$", words[-1:][0]):
            p = float( words[-1:][0] )
            words = words[:-1]
            if p < 0.01:
                print "p too small, setting to 0.01"
                p = 0.01
        print words, p
        pairs = []
        for i in range(len(words)):
            for j in range(len(words)):
                if i != j:
                    add_error(words[i], words[j], p, len(words))
            
print
for err in vk_errors:
    try:
        print err, len( vk_errors[err] ), Counter( vk_errors[err] )
    except:
        print err, len( vk_errors[err] ), vk_errors[err] 
print
        
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
            numindexes = len(indexes)
            # create one or two errors per line
            randomidxs = []
            if numindexes > 0 and numindexes < 3:
                randomidxs = [ indexes[ int( r.uniform(0, numindexes) ) ] ] #choose one,
            elif numindexes > 0 and numindexes > 2:
                randomidxs = random.sample(set(indexes), 2) #take two random
            for randomidx in randomidxs:
                w = words[ randomidx ] #the word from the line
                numerrs = len( vk_errors[w] ) #number of possible errors
                randerr = int( r.uniform(0, numerrs) ) #choose one of them
                confused = vk_errors[w][randerr] #the confused word
                words[ randomidx ] = confused 
                ld = damerau(w, confused)
                if ld > 0: #could be the original word in the array for low prob changes
                    print "CHANGE", w, confused, ld
                    try:
                        changed[ confused ] += 1
                    except:
                        changed[ confused ] = 1
                    try:
                        lds[ld] += 1
                    except:
                        lds[ld] = 1
                    made_changes += 1
            new_line = ' '.join(words)
            fo.write(new_line+"\n")

print "Made", made_changes, "changes out of", poss_changes, "possible changes."


for ld in vk_ld_counts:
    print "CFLD", ld, vk_ld_counts[ld]

for change in changed:
    print "FREQ", change, changed[change]
for ld in lds:
    print "LD", ld, lds[ld]
