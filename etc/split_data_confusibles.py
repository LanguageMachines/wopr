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
creates timblserver ini file
creates stimpy.py code to include
"""

trfile   = None #training file
cfile    = None #file with confusibles
multiple = False #not used
offset   = -1 #last word in "a b c", the target
sfile    = "conf_train.wopr.sh" #script fil
binary   = False #make +/- classifiers

w_id = "EXP01" #should be supplied to wopr as well

wopr_path  = "/exp2/pberck/wopr/wopr"
wopr_timbl = '"-a1 +D"'

try:
    opts, args = getopt.getopt(sys.argv[1:], "bc:f:i:o:s:t:w:", ["file="])
except getopt.GetoptError, err:
    print str(err)
    sys.exit(1)
for o, a in opts:
    if o in ("-f", "--file="):
        trfile = a 
    elif o in ("-c"): 
        cfile = a
    elif o in ("-b"): 
        binary = True
    elif o in ("-o"): 
        offset = int(a)
    elif o in ("-i"): 
        w_id = a
    elif o in ("-s"): 
        sfile = a
    elif o in ("-t"): 
        wopr_timbl = '"'+a+'"' #quotes!
    elif o in ("-w"): 
        wopr_path = a
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
# if binary, we need a negative classifier/filelist as well
rev = {}
tr_openfiles = {}
trigger_counter = {}
for c in confusibles:
    counter = c[0]
    tr_c_file = trfile + "_" + w_id + ".cs" + str(counter) #confusible set
    #print tr_c_file
    tr_openfiles[tr_c_file] = open(tr_c_file, "w")
    words = c[1]
    print words
    for w in words:
        rev[w] = tr_c_file
        trigger_counter[w] = 0

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
            #if binary, we change target to '+'
            if binary:
                words[-1] = '+'
                line = " ".join(words)
            of.write(line+"\n")
            trigger_counter[trigger] += 1
        index += 1

"""
Once more, add negative examples. order is not important.
number is...count number in + files? Same -ve example in more
than one file? We should only run this script with one set
if doing binary
"""
if binary:
    negs = 0
    # use trigger counter to count negs?
    with open(trfile, "r") as f:
        index = 0
        for line in f:
            line = line[:offset]
            words = line.split()
            trigger = words[offset]
            if trigger not in rev:
                for tr in tr_openfiles:
                    of = tr_openfiles[tr] #get filepointer
                    words[-1] = '-'
                    line = " ".join(words)
                    of.write(line+"\n")
                    negs += 1
            index += 1

#close all
for fp in tr_openfiles:
    of = tr_openfiles[fp]
    of.close()

ctx_it_re = re.compile(".*l(\d)t1r(\d)$")
ctx_re = re.compile(".*l(\d)r(\d)$")
lc = None
rc = None
it = None

m = ctx_it_re.match(trfile)
if m:
    lc = int(m.group(1))
    rc = int(m.group(2))
    it = 1
m = ctx_re.match(trfile)
if m:
    lc = int(m.group(1))
    rc = int(m.group(2))
    it = 0

with open("make_ibases_"+w_id+".sh", "w") as of_ib:
    with open("configfile_"+w_id+".txt", "w") as of_cf:
        with open("tserver_"+w_id+".ini", "w") as of_ts:
            with open("pyserver_"+w_id+".py", "w") as of_py:
                idx = 0
                of_ts.write("port=2000\n")
                of_ts.write("maxconn=20\n")
                of_py.write( "s1=TServers(\"localhost\",2000)\n" )
                if it == 1:
                    of_py.write( "# it:1\n" )
                for c in confusibles:
                    counter = c[0]
                    words = c[1]
                    print words
                    # check if none of the counters is 0...
                    counts = [ trigger_counter[w] for w in words ]
                    positives = [e for e in counts if e > 0]
                    print counts
                    if len(positives) < 2:
                        print "Useless classifier."
                        continue
                    tr_c_file = trfile + "_" + w_id + ".cs" + str(counter)
                    #
                    print wopr_path+" -l -r make_ibase -p filename:"+tr_c_file+",id:"+w_id+",timbl:"+wopr_timbl
                    of_ib.write(wopr_path+" -l -r make_ibase -p filename:"+tr_c_file+",id:"+w_id+",timbl:"+wopr_timbl+"\n")
                    #
                    # utexas.1000000.l2r0_EXP01.cs19_-a1+D.ibase
                    timbl_compact = wopr_timbl.replace(" ", "").replace("\"", "")
                    ibasefile = tr_c_file+"_"+timbl_compact+".ibase" #outside of wopr, we create our own filenames.    
                    print ibasefile, " ".join(c[1])
                    of_cf.write(ibasefile+" "+" ".join(c[1])+"\n")
                    # File for tibleservers.
                    #port=2000
                    #maxconn=2
                    #wpred="-i DTI.1e6.10000.l2r2_-a1+D.ibase -a1 +D +vdb+di"
                    bits = wopr_timbl.split()
                    id_str = '{0:03n}'.format( idx )
                    of_ts.write(w_id+id_str+"=\"-i "+ibasefile+" "+wopr_timbl.replace("\"", "")+" +vdb+di\"\n")
                    ty = 1
                    if it == 1:
                        ty = 2
                    ts = "s1"
                    of_py.write( w_id+id_str+"=Classifier(\""+w_id+id_str+"\", "+str(lc)+", "+str(rc)+", "+str(ty)+", "+ts+")\n" )
                    words = [ "\""+word+"\"" for word in words ]
                    triggers = ",".join(words)
                    of_py.write( w_id+id_str+".set_triggers(["+triggers+"])\n" )
                    of_py.write( "s1.add_classifier("+w_id+id_str+")\n" )
                    idx += 1
