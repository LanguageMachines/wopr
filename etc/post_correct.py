#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# run "correct" on output from wex/multiple classifiers.
#
# ...mg:
# passion for school and , -5.682 D dflt ic k 2 0 30 68 0 [ , 14 . 13 choice 6 uniforms 3 problems 3 ]
#                                   ----                  ----- this 

from sys import argv
import time, datetime
import os
import re
import sys
from xml.dom import minidom
from xml.dom.minidom import getDOMImplementation
from xml.dom.minidom import Document
import datetime
import time
from time import strptime
from time import asctime
import getopt
import tempfile

def levenshtein(a,b):
    "Calculates the Levenshtein distance between a and b."
    n, m = len(a), len(b)
    if n > m:
        # Make sure n <= m, to use O(min(n,m)) space
        a,b = b,a
        n,m = m,n
        
    current = range(n+1)
    for i in range(1,m+1):
        previous, current = current, [i]+[0]*n
        for j in range(1,n+1):
            add, delete = previous[j]+1, current[j-1]+1
            change = previous[j-1]
            if a[j-1] != b[i-1]:
                change = change + 1
            current[j] = min(add, delete, change)
            
    return current[n]


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
            current_row.append(min(insertions, deletions, substitutions))
        previous_row = current_row
 
    return previous_row[-1]

# ---

"""
tstart = datetime.datetime.now()
print tstart
for x in xrange(100000):
    levenshtein(argv[1],argv[2])
tend = datetime.datetime.now()
print tend-tstart

tstart = datetime.datetime.now()
print tstart
for x in xrange(100000):
    levenshtein_wp(argv[1],argv[2])
tend = datetime.datetime.now()
print tend-tstart
"""

# ---

try:
    opts, args = getopt.getopt(sys.argv[1:], "f:", ["file="])
except getopt.GetoptError, err:
    # print help information and exit:
    print str(err) # will print something like "option -a not recognized"
    #usage()
    sys.exit(2)
for o, a in opts:
    if o in ("-f", "--file"):
        mg_file = a
    elif o in ("-i", "--info"):
        info_only = True
    else:
        assert False, "unhandled option"

f = open( mg_file, 'r' )
for line in f.readlines():
    #print line[:-1]
    # we need to know context
    parts = line.split(' ');
    wclass = parts[3]
    #print wclass
    m = re.search( '\[ (.*) \]', line ) 
    if m != None:
        distr = m.group(1)
        bits = distr.split(' ')
        #bits2 = filter(lambda x:x.isalpha(), bits)
        bits2 = [x for x in bits if not x.isdigit()] #this misses predicted numbers
        for a in bits2:
            ld =  levenshtein(wclass, a)
            if len(a) > 2 and ld == 1:
                print wclass, a
