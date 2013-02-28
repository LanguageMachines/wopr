#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# Make confusibles data sets, create wopr script.
# Workflow:
# first create dataset in right format, e.g l2r2:
# wopr -r window_lr -p filename:austen.txt,lc:2,rc:2
#
# make a list with cunfusables, clist
# create confusible sets from the resulting windowed data:
# python confusedata.py -c clist -f austen.test.l2r2 >wopr.script
#
# Run the resulting wopr.script, with desired timbl settings:
# wopr -s wopr.script -p timbl:"-a1 +D"
#
# run dir_to_tspy.py to create a ts.out for timblserver
#
# Use stimpy.py to talk to the timblservers.
#
import os
import re
import sys
import datetime
import time
from time import strptime
from time import asctime
import getopt
from time import sleep
from itertools import islice, izip
from operator import itemgetter
import copy

# ----

class Counter():
    def __init__(self, start):
        self.num = start
    def getnr(self):
        return '{0:04d}'.format(self.num)
    def incnr(self):
        self.num += 1
    def getinc(self):
        res = '{0:04d}'.format(self.num)
        self.incnr()
        return res

class CObj():
    def __init__(self, cl):
        self.clist = cl
        self.filename = ""
        self.fh = None
        self.hits = 0
        self.classcount = dict( ((cl,0) for cl in cl) )
    def openfile(self):
        self.fh = open( self.filename, "w" )
    def closefile(self):
        self.fh.close()
    def check(self, t, l):
        if t in self.clist:
            self.fh.write(l)
            self.hits += 1
            self.classcount[t] += 1
    def wopr(self):
        #print "set filename:"+self.filename
        #print "run: make_ibase"
        #print "filename:"+self.filename
        return "wopr -l -r make_ibase -p filename:"+self.filename+",timbl:\"${TIMBL}\""
    def woprscript_ini(self, srv, prt):
        # note the unset:ibase : otherwise wopr remembers the name of the first one,
        #      and will refuse to make more (file exists).
        res = "unset:ibasefile"+"\n"+"set:filename:"+self.filename+"\n"
        res += "run: make_ibase"+"\n"
        #res += "extern: echo $timbl $ibasefile \n"
        res += "extern: echo \"# Added by script.\" >> $kvs\n"
        res += "extern: echo [ts"+srv+"] >> $kvs\n"
        res += "extern: echo cmd=/Users/pberck/work/local/bin/timblserver -i $ibasefile $timbl +vdb+di -S"+prt+" --daemonize=no >> $kvs\n"
        return res
    def woprscript_ts(self, srv):
        # note the unset:ibase : otherwise wopr remembers the name of the first one,
        #      and will refuse to make more (file exists).
        res  = "unset:ibasefile"+"\n"+"set:filename:"+self.filename+"\n"
        res += "run: make_ibase"+"\n"
        return res
                    
# ----

def dbg(*strs):
    if debug:
        print >> sys.stderr, "DBG:", "".join(strs)

def find(f, seq):
  """Return first item in sequence where f(item) == True."""
  for item in seq:
    if f(item): 
      return item

def work(tfile, cfile, c):
    counter = Counter(c)
    clist = [ line.split() for line in open(cfile) ]
    cobjs = [ CObj(cl) for cl in clist ]
    for c in cobjs:
        c.filename = tfile+'.'+"c"+counter.getinc()
        #print c.filename
        c.openfile()
    tfh = open( tfile, 'r' )
    for line in tfh:
        words = line[:-1].split()
        target = words[-1:][0]
        for c in cobjs:
            c.check(target, line)
    [ (c.closefile(),c.filename,c.hits) for c in cobjs ]
    return cobjs
    
# ----

def ans_str(ans):
    return '{0} {1} {2:.4f} {3:.4f} {4:.4f} {5} {6} _ _ {7:d} {8:.0f} _'.format(*ans) + topn_str(ans[9])

"""
port=7000
maxconn=10
dimin0="-aIB1 +vdi+db+n -f dimin.train"
dimin1="-aIGTREE +vdi+db +D -f dimin.train"
dimin2="-aTRIBL +vdi+db+n -q3 -f dimin.train"

port=2000
maxconn=2
ts0="-i austen.train.l2r2.c0003_-a4+D.ibase -a4 +D +vdb+di"
ts1="-i austen.train.l2r2.c0002_-a4+D.ibase -a4 +D +vdb+di"

Welcome to the Timbl server.
available bases: ts0000 ts0001 ts0002 ts0003
b ts0000
selected base: 'ts0000'
c it was house or _
CATEGORY {to} DISTRIBUTION { to 28.0000, too 10.0000 } DISTANCE {0.0658311}
b ts0001
selected base: 'ts0001'
c it was house or _
CATEGORY {then} DISTRIBUTION { then 2.00000 } DISTANCE {0.105978}
b ts0002
selected base: 'ts0002'
c it was house or _
CATEGORY {their} DISTRIBUTION { their 5.00000 } DISTANCE {0}
b ts0003
selected base: 'ts0003'
c it was house or _
CATEGORY {one} DISTRIBUTION { one 26.0000 } DISTANCE {0.0139275}
"""

# -----------------------------------------------------------------------------
# ----- MAIN starts here
# -----------------------------------------------------------------------------

if __name__ == "__main__":

    debug        = False
    verbose      = 0
    tfile        = None
    cfile        = None
    lc           = 0
    rc           = 0
    count        = 0
    
    try:
        opts, args = getopt.getopt(sys.argv[1:], "dc:f:t:v:", ["lc=","rc=","count="])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        #usage()
        sys.exit(2)
    for o, a in opts:
        if o in ("-c"):
            cfile = a
        elif o in ("--count"):
            count = int(a)
        elif o in ("-f"):
            tfile = a
        elif o in ("-v"):
            verbose = int(a)
        elif o in ("--lc"):
            lc = int(a)
        elif o in ("--rc"):
            rc = int(a)
        else:
            print o,a
            assert False, "unhandled option"

    cobjs = work(tfile,cfile,count)
    #remove with hits = 0, or one classcount of 0
    #
    '''
    print "TIMBL=\"-a1 +D\""
    for c in cobjs:
        print "#",c.filename, repr(c.clist)
        print c.wopr()
    '''
    #
    # wopr.script to create ibases and process.ini
    counter = Counter(count)
    print "# -*- shell-script -*-"
    print "#"
    # we need a default ibase
    res  = "unset:ibasefile"+"\n"+"set:filename:"+tfile+"\n"
    res += "run: make_ibase"+"\n"
    print res
    #
    tsrvs = Counter(count)
    for c in [c for c in cobjs if c.hits > 0]:
        # count the number of classes with 0 instances
        zeroes = len( [ w for w in c.classcount if c.classcount[w] == 0 ] )
        # if there is a class with 0, we skip this classifier
        # (maybe a low % should also be skipped)
        if zeroes == 0:
            print c.woprscript_ts(tsrvs.getinc())
    
