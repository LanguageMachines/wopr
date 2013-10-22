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

Examples:
python2.7 split_data_confusibles.py -f utexas.10e6.dt.cf05.1e6.l4t1r4 -c preps_hoo2013.txt -t "-a1 +D" -w "wopr" -i

(scootaloo)
python split_data_confusibles.py -f utexas.10e6.dt.1e6.l4r4 -c dets_hoo2013.txt -b -w wopr -i UT1e6_l4r4_DETSPN
python split_data_confusibles.py -f utexas.10e6.dt.1e6.l4r4 -c preps_hoo2013.txt  -w wopr -i PREPS
"""

trfile    = None #training file
cfile     = None #file with confusibles
multiple  = False #not used
offset    = -1 #last word in "a b c", the target
sfile     = "conf_train.wopr.sh" #script fil
binary    = False #make +/- classifiers
w_id      = "EXP01" #should be supplied to wopr as well

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

def upcase_first_letter(s):
    return s[0].upper() + s[1:]
def lower_first_letter(s):
    return s[0].lower() + s[1:]
def toggle_first_letter(s):
    if s[0].isupper():
        return lower_first_letter(s)
    return upcase_first_letter(s)
    
confusibles = []
counter = 0
with open(cfile, "r") as f:
    for line in f:
        if line == "" or line[0] == "#":
            continue
        line = line[:-1]
        words = line.split()
        words2 = [ toggle_first_letter(w) for w in words]
        confusibles.append((counter,words+words2))
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
If more than one set, do the following per set, re-reading from the
dataset from 0 every time to add negatives.
"""
'''
skip = 2
if binary:
    negs = 0
    for c in confusibles:
        print c
        counter = c[0]
        tr_c_file = trfile + "_" + w_id + ".cs" + str(counter) #confusible set
        print tr_openfiles[tr_c_file] 

        want_negs = sum(trigger_counter.values())
        pn_factor = 1.0
        want_negs = int(want_negs*pn_factor)
        print want_negs

        with open(trfile, "r") as f:
            for line in f:
                if negs >= want_negs:
                    break
                if random.randint(0,9) < skip:
                    continue
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
sys.exit(1)
'''

"""
"Here is (the) missing det."  MISSING
"I am going (the) home."      REDUNDANT

Test on MISSING - no word on focus/target pos: here-is-//-missing-det:MISSING, no focus pos
Test on REDUNDANT - word on focus/target pos: am-going-/the/-home-.:the, with focus pos, classification should be DETRED


Train on MISSING - normal instances where PREP is a target (here should be a PREP between l2r2)
Train on REDUNDANT -normal instances where NOT-PREP is a target (there should not be a PREP between l2r2)

I am going home .
I go to the market .

l2r2
_ _ am going I
_ I going home am
I am home . going  PREPRED-NEG or DETRED-NEG (between "I am" and "home ." not a PREP or DET) with focus/target
am going . _ home
going home _ _ .
_ _ go to I
_ I to the go       PREPMIS-NEGATIVE, DETMIS-NEGATIVE
I go the market to  PREPMIS-POSITIVE
go to market . the  DETMIS-POSITIVE
to the . _ market
the market _ _ .

"""

skip = 2
want_negs = sum(trigger_counter.values())
pn_factor = 1.0
want_negs = 10*int(want_negs*pn_factor)
print want_negs
if binary:
    negs = 0
    # use trigger counter to count negs?
    with open(trfile, "r") as f:
        index = 0
        for line in f:
            if negs >= want_negs:
                break
            if random.randint(0,9) < skip:
                continue
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
    if want_negs > negs:
        print "Did not get all negatives (",want_negs-negs,"left )"

        
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
