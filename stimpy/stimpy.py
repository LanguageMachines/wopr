#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# Simple TIMbleserver PYthon Interface
#
import os
import re
import sys
from xml.dom import minidom
import datetime
import time
from time import strptime
from time import asctime
from math import sqrt,sin,cos,asin,pi
import getopt
import math
import codecs
import socket
from time import sleep
from itertools import islice, izip
from operator import itemgetter
import random
import copy
import math;

# ---

debug = False
def dbg(*strs):
    if debug:
        print >> sys.stderr, "DBG:", "".join(str(strs))

def find(f, seq):
  """Return first item in sequence where f(item) == True."""
  for item in seq:
    if f(item): 
      return item
  
'''
[ _ _ _ the quick brown fox _ _ _ ]
        ^start
        [ pos-LC : pos+LC ]
'''
def window_lr( str, lc, rc ):
    str_ar = str.split()
    # words = ["_" for x in range(lc-1)] + ["<S>"] + str_ar + ["</S>"] + ["_" for x in range(rc-1)]
    words = ["_" for x in range(lc)] + str_ar + ["_" for x in range(rc)]
    result = []
    for i in range(len(str_ar)):
        #              shift to right index by added lc
        res = words[i:i+lc] + words[i+1+lc:i+rc+1+lc] + [str_ar[i]]
        #print i, words[i:i+lc],words[i+1+lc:i+rc+1+lc],"=",str_ar[i]
        #print " ".join(res)
        result.append( " ".join(res) )
    return result

def window_to( str, lc, o ): #offset
    str_ar = str.split()
    # words = ["_" for x in range(lc-1)] + ["<S>"] + str_ar + ["</S>"] + ["_" for x in range(rc-1)]
    words = ["_" for x in range(lc)] + str_ar
    result = []
    for i in range(len(str_ar)-o):
        #              shift to right index by added lc
        res = words[i:i+lc] + [str_ar[i+o]]
        #print i, words[i:i+lc],words[i+1+lc:i+rc+1+lc],"=",str_ar[i]
        #print " ".join(res)
        result.append( " ".join(res) )
    return result
        
class TServers():
    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.ibs = []
        self.classifiers = []
        self.s = None
        self.file = None
        self.lex = {}
        self.topn = 3
        self.triggers = {}
        try:
            self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.s.connect((self.host, self.port))
            self.file = self.s.makefile("rb") # buffered
            #self.s.settimeout(1)
            #print self.s.gettimeout()
        except:
            print "Error, no timblserver"
            sys.exit(8)
        self.readline() #available bases
                        #print "1",self.data
        self.readline() #available bases
                        #print "2",self.data
        #self.readline() #available bases
        #print "3",self.data

    def readline(self):
        "Read a line from the server.  Strip trailing CR and/or LF."
        CRLF = "\r\n"
        if not self.file:
            return
        s = self.file.readline()
        if not s:
            raise EOFError
        if s[-2:] == CRLF:
            s = s[:-2]
        elif s[-1:] in CRLF:
            s = s[:-1]
        self.data = s

    def sendline(self,l):
        try:
            self.s.sendall(l)
        except socket.error:
            print "Socket kapot."
        
    # classify one instance, classifier c
    def classify( self, c, i ):
        if not self.s:
            return
        self.i = i
        tokens = self.i.split()
        self.target = tokens[-1]
        #what do we return? Own class for ib?
        self.combined = []
        c.error = False
        #try:
        self.sendline("b "+c.name+"\n")
        self.readline()
        # check result!
        m = re.search( r'ERROR {', self.data )
        if m:
            #print self.data
            c.ans = []
            c.error = True
            return
        self.sendline("c "+i+"\n")
        self.readline()
        m = re.search( r'ERROR {', self.data )
        if m:
            #print self.data
            c.ans = []
            c.error = True
            return
        self.grok( self.data )
        c.ans = self.ans
        self.combined += self.distr[0:self.topn]
        '''except:
            print "Unexpected error:", sys.exc_info()[0]
            c.error = True
            return
        '''
        self.combined.sort(key=lambda tup: tup[1], reverse=True)
        dbg( self.combined )

        # classify with triggers.
    def classify_tr(self, i):
        if not self.s:
            return
        self.i = i
        tokens = self.i.split()
        self.target = tokens[-1]
        if self.target in self.triggers:
            c = self.triggers[self.target]
            return self.classify(c,i)
        return []

    def get_trigger(self, i):
        tokens = i.split()
        target = tokens[-1]
        if target in self.triggers:
            c = self.triggers[target]
            return c
        return None
    
    def grok( self, tso ):
        '''
        Parse the TimblServer output
        '''
        #CATEGORY {concerned} DISTRIBUTION { concerned 2.00000, volatile 1.00000 } DISTANCE {0}
        dbg(tso)
        # error checking
        m = re.search( r'ERROR {', tso )
        if m:
            print tso
            print self.i
            self.data = ""
            self.ans = []
            self.distr = []
        m = re.search( r'CATEGORY {(.*)} DISTRIBUTION { (.*) } DISTANCE {(.*)}', tso )
        #print m.group(1) #concerned
        self.classification = m.group(1)
        #print m.group(2) #concerned 2.00000, volatile 1.00000
        #print m.group(3) #0 distance
        self.distance = float(m.group(3))
        bits = m.group(2).split()
        pairs = zip(bits[::2], bits[1::2])
        #remove silly commas from frequencies
        distr = {}
        sum_freqs = 0
        distr_count = 0
        for pair in pairs:
            #print pair[0], pair[1].rstrip(",")
            sum_freqs += float(pair[1].rstrip(","))
            distr_count += 1
            distr[ pair[0] ] = float(pair[1].rstrip(","))
        #print sorted( distr.iteritems(), key=itemgetter(1), reverse=True )[0:5]
        #print "dc,sf",distr_count, sum_freqs
        self.distr = copy.deepcopy(\
                    sorted( distr.iteritems(), key=itemgetter(1), reverse=True ) )
        self.distr_count = distr_count
        self.sum_freqs = sum_freqs
        self.distr = [(w,f,f/self.sum_freqs) for (w,f) in self.distr]
        self.logs = [p * math.log(p,2) for (w,f,p) in self.distr]
        self.entropy = -sum(d for d in self.logs)
        #self.sm = sum(d for (a,b,c,d) in self.distr)
        self.pplx = math.pow(2, self.entropy)
        self.p = 0
        self.indicator = "??"
        self.ku = "u"
        lexf = self.in_lex(self.target) * 1.0
        if lexf:
            self.ku = "k"
        if self.classification == self.target:
            self.p = (self.distr[0])[2]
            self.indicator = "cg"
        else:
            #print "ID",self.in_distr(self.target) #faster than next line?
            tmp = [(w,f,p) for (w,f,p) in self.distr if w == self.target]
            if tmp != []:
                tmp = tmp
                self.p = tmp[0][2]
                self.indicator = "cd"
            else:
                self.indicator = "ic"
                if lexf:
                    self.p = lexf / self.lex_sumfreq
        #if self.p == 0: self.p = 1.8907e-06 #p(0) should be calculated
        try:
            self.log2p = math.log(self.p,2) #word logprob
            self.wlpx  = math.pow(2,-self.log2p)
        except ValueError:
            self.log2p = -99.0
            self.wlpx  = 0#pow(2,32) #0 #should be inf, ok for logscale y plot
        topn = self.distr[0:self.topn]
        self.ans = (self.i, self.classification, self.log2p, self.entropy, self.wlpx,\
                    self.indicator, self.ku, self.distr_count, self.sum_freqs,topn)

    def in_distr(self, w):
        getword = itemgetter(0)
        res = find( lambda ding: ding[0] == w, self.distr )
        return res

    def read_lex(self, lexfile):
        self.lex = dict( ( line.split()[0], int(line.split()[1])) for line in open(lexfile))
        self.lex_sumfreq = sum(self.lex.itervalues())
        self.lex_count = len(self.lex)

    def in_lex(self, w):
        try:
            return self.lex[w]
        except KeyError:
            return 0
    def add_triggers(self, c, t):
        for trigger in t:
            self.triggers[trigger] = c
    
# Classifier should talk to timblserver
class Classifier():
    def __init__(self, name, lc, rc, ts):
        self.name = name
        self.lc = lc
        self.rc = rc
        self.to = 0
        self.ts = ts #Timblserver
        self.ans = None  #one instance answer
        self.error = False
        self.triggers = []
    def window_lr(self, s):
        if self.to == 0:
            res = window_lr(s, self.lc, self.rc)
        else:
            res = window_to(s, self.lc, self.to)
        return res
    def set_triggers(self, t):
        self.triggers = t
        self.ts.add_triggers(self, t)
    def classify_i(self, i):
        self.ts.classify(self, i)
        return self.ans
    def classify_s(self, s):
        return [ self.classify_i(i) for i in self.window_lr(s) ]

# ----

# Output from wopr:
# instance+target classification log2prob entropy word_lp guess k/u md mal dist.cnt dist.sum RR ([topn])
# stympy:
# (i, classification, log2p, entropy, wlpx, indicator, ku, distr_count, sum_freqs, topn)

def ans_str(ans):
    if ans and len(ans) == 10:
        return '{0} {1} {2:.4f} {3:.4f} {4:.4f} {5} {6} _ _ {7:d} {8:.0f} _'.format(*ans) + topn_str(ans[9])
    return None

def topn_str(tns):
    if len(tns) == 0:
        return ""
    res = " [ "
    for tn in tns:
        s = '{0} {1:.0f} '.format(*tn)
        res += s
    res += "]"
    return res

#file with ans_str(results):
#plot [][] "plt" using 6:xticlabels(3) with lp

# -----------------------------------------------------------------------------
# ----- MAIN starts here
# -----------------------------------------------------------------------------

if __name__ == "__main__":
    print "Python interface for timblsercer."
