
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
"""

def levenshtein_wp(s1, s2):
    if len(s1) < len(s2):
        return levenshtein(s2, s1)
    if not s1:
        return len(s2)
 
    previous_row = xrange(len(s2) + 1)
    for i, c1 in enumerate(s1):
        current_row = [i + 1]
        for j, c2 in enumerate(s2):
            insertions = previous_row[j + 1] + 1 # j+1 instead of j since previous_row and current_row are one character longer
            deletions = current_row[j] + 1       # than s2
            substitutions = previous_row[j] + (c1 != c2)
            if j-i == 1: #this 'correction' makes swap LD:1
                substitutions -= 1
            current_row.append(min(insertions, deletions, substitutions))

        previous_row = current_row
 
    return previous_row[-1]

afile = None
vkfile = "ValkuilErrors.1.0_ns"
prob = 2
lex = None
min_l = 3 #minimum length of word >= min_l
max_ld = 1 #maximum LD <= max_ld

try:
    opts, args = getopt.getopt(sys.argv[1:], "d:f:l:m:v:p:", ["file="])
except getopt.GetoptError, err:
    print str(err)
    sys.exit(1)
for o, a in opts:
    if o in ("-f", "--file="):
        afile = a 
    elif o in ("-v"): 
        vkfile = a
    elif o in ("-l"): 
        lex = a
    elif o in ("-m"): 
        min_l = int(a)
    elif o in ("-d"): 
        max_ld = int(a)
    elif o in ("-p"): 
        prob = int(a)
    else:
        assert False, "unhandled option"

lexicon = {}
lex_errs = {}
if lex:
    lexicon = dict( ( line.split()[0], int(line.split()[1])) for line in open(lex))

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
            err_in_lex = False
            if lex:
                if e in lexicon:
                    err_in_lex = True
                    try:
                        lex_errs[e] += 1
                    except:
                        print "LEXICON", e
                        lex_errs[e] = 1
            ld = levenshtein_wp(w, e)
            if not err_in_lex:#reverse for confusibles only,
                if ld <= max_ld and len(w) >= min_l:
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
            # create one or two errors per line
            randomidxs = []
            if num < prob and numindexes > 0 and numindexes < 3:
                randomidxs = [ indexes[ int( r.uniform(0, numindexes) ) ] ] #choose one,
            elif num < prob and numindexes > 0 and numindexes > 2:
                randomidxs = random.sample(set(indexes), 2) #take two random
            for randomidx in randomidxs:
                w = words[ randomidx ] #the word from the line
                numerrs = len(vk_errors[w]) #number of possible errors
                randerr = int( r.uniform(0, numerrs) ) #choose one of them
                words[ randomidx ] = vk_errors[w][randerr]
                print "CHANGE", w, words[ randomidx ] 
                made_changes += 1
            new_line = ' '.join(words)
            fo.write(new_line+"\n")
print "Made", made_changes, "changes out of", poss_changes, "posible changes."

if lex:
    print "Lexicon contains",len(lex_errs),"errors."
    #print repr(lex_errs)