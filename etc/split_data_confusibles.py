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
Takes a windowed data set and a confusible file.

create ibases, plus control file:
ibasename timbl confusibles

"""

trfile    = None #training file
cfile    = None #file with confusibles
multiple = False #not used
offset   = -1 #last word in "a b c", the target
sfile    = "conf_train.wopr.sh" #script fil

w_id = "EXP01"

wopr_path  = "/exp2/pberck/wopr/wopr"
wopr_timbl = '"-a1 +D"'

try:
    opts, args = getopt.getopt(sys.argv[1:], "c:f:o:s:", ["file="])
except getopt.GetoptError, err:
    print str(err)
    sys.exit(1)
for o, a in opts:
    if o in ("-f", "--file="):
        trfile = a 
    elif o in ("-c"): 
        cfile = a
    elif o in ("-o"): 
        offset = int(a)
    elif o in ("-s"): 
        sfile = a
    else:
        assert False, "unhandled option"

confusibles = []
counter = 0
with open(cfile, "r") as f:
    for line in f:
        if line == "" or line[0] == "#":
            continue
        line = line[:-1]
        words = line.split()
        confusibles.append((counter,words))
        counter += 1

#print confusibles

# open a file for each set, 
# create reverse hash from confusible to => counter
# create hash with openfiles
rev = {}
tr_openfiles = {}
indexfiles = {}
for c in confusibles:
    counter = c[0]
    tr_c_file = trfile + "_" + w_id + ".cs" + str(counter) #confusible set
    #print tr_c_file
    tr_openfiles[tr_c_file] = open(tr_c_file, "w")
    ifile = tr_c_file + ".i"
    indexfiles[ifile] = open(ifile, "w")
    words = c[1]
    for w in words:
        rev[w] = tr_c_file

#print rev

# Open windowed file, take word at offset to check in confusible list
with open(trfile, "r") as f:
    index = 0
    for line in f:
        line = line[:offset]
        words = line.split()
        trigger = words[offset]
        if trigger in rev:
            #print trigger, rev[trigger]
            fp = rev[trigger] #get filename
            of = tr_openfiles[fp] #get filepointer
            of.write(line+"\n")
            of = indexfiles[fp+".i"] #get filepointer for index
            of.write(str(index)+"\n")
        index += 1
        
#close all
for fp in tr_openfiles:
    of = tr_openfiles[fp]
    of.close()
    of = indexfiles[fp+".i"]
    of.close()

for c in confusibles:
    counter = c[0]
    tr_c_file = trfile + "_" + w_id + ".cs" + str(counter)
    #
    print wopr_path+" -l -r make_ibase,kvs -p filename:"+tr_c_file+",id:"+w_id+",timbl:"+wopr_timbl
    #
    # utexas.1000000.l2r0_EXP01.cs19_-a1+D.ibase
    timbl_compact = wopr_timbl.replace(" ", "").replace("\"", "")
    ibasefile = tr_c_file+"_"+timbl_compact+".ibase" #outside of wopr, we create our own filenames.    
    print ibasefile, w, " ".join(c[1])
    
