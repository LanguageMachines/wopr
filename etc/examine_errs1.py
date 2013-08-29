#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
import time, datetime
import os
import re
import sys
import datetime
import time
import getopt
import tempfile
from math import log

'''
ERROR: being should be begin
GOODSUGG being -> begin
BADSUGG led -> lead (should be: led)
ERROR: sight should be site
WRONGSUGG sight -> cite (should be: site)
ERROR: they're should be there
WRONGSUGG they're -> their (should be: there)
BADSUGG than -> then (should be: than)
'''

# Hold the info for a confusible
class Matrix():
    def __init__(self, word):
        self.confusible = word
        self.GS = 0
        self.WS = 0
        self.NS = 0
        self.BS = 0
        #print "MATRIX:", word
        self.corrected_from = []
        self.corrected_as = []
    def add_GS(self, error):
        # GOODSUGG being -> begin
        # add the original error to list
        self.GS += 1
        self.corrected_from.append(error)
    def add_NS(self):
        self.NS += 1
    def add_WS(self, error):
        # WRONGSUGG sight -> cite (should be: site)
        # should that be here?
        self.WS += 1
        self.corrected_as.append(error)
    def add_BS(self, correction):
        # BADSUGG led -> lead (should be: led)
        self.BS += 1
    def total(self):
        return self.GS+self.WS+self.NS
    def to_str(self):
        ans = self.confusible+": TOT: "+str(self.total())+" GS:"+str(self.GS)+" WS:"+str(self.WS)+" NS:"+str(self.NS)
        return ans
    def to_latex(self):
        ans = self.confusible+" & \\tnum{"+str(self.total())+"} & \\tnum{"+str(self.GS)+"} & \\tnum{"+str(self.WS)+"} & \\tnum{"+str(self.NS)+"} & \\tnum{"+str(self.BS)+"}"
        return ans
    
all_files = None
cfile = None

try:
    opts, args = getopt.getopt(sys.argv[1:], "c:f:", ["file="])
except getopt.GetoptError, err:
    #print str(err)
    sys.exit(2)
for o, a in opts:
    if o in ("-f", "--file"):
        all_files = [ a ]
    elif o in ("-c", "--cfile"):
        cfile = a 
    else:
        assert False, "unhandled option"

cf_info = {}

confusibles = []
counter = 0
with open(cfile, "r") as f:
    for line in f:
        if line == "" or line[0] == "#":
            continue
        line = line[:-1]
        words = line.split()
        confusibles.append( words )

print confusibles
        
for filename in all_files:
    with open(filename, 'r') as f:
        file_lines = f.readlines()
    w_error = None
    w_orig = None
    for line in file_lines:
        matched   = False
        # Match for 
         #ERROR: they're should be there
         #GOODSUGG there -> their
        m = re.match( "ERROR: (.*?) should be (.*?)$", line )
        if m:
            #print m.groups()
            w_error = m.group(1)
            w_orig  = m.group(2)
            matched  = True
            if w_orig not in cf_info:
                m = Matrix(w_orig)
                cf_info[w_orig] = m
                #when it was X it was corrected as Y
        m = re.match( "GOODSUGG (.*?) \-> (.*?)$", line )
        if m:
            #print line
            w_error = m.group(1)
            w_orig  = m.group(2)
            try:
                m = cf_info[w_orig]
                m.add_GS(w_orig)
            except KeyError:
                print "ERROR", line
                sys.exit(1)
                #BADSUGG between -> among (should be: between)
        m = re.match( "BADSUGG (.*?) \-> (.*?) \(", line )
        if m:
            #print line
            w_orig  = m.group(1)
            w_error = m.group(2)
            #print w_orig, w_error
            try:
                m = cf_info[w_orig]
                m.add_BS(w_error)
            except KeyError:
                m = Matrix(w_orig)
                cf_info[w_orig] = m
                m.add_BS(w_error)
        #WRONGSUGG sight -> cite (should be: site)
        #in the text we have SIGHT, corrected to CITE, should have been SITE
        m = re.match( "WRONGSUGG (.*?) \-> (.*?) \(should be: (.*?)\)", line )
        if m:
            #print line
            w_orig  = m.group(3)
            w_error = m.group(2)
            #print w_orig, w_error
            try:
                m = cf_info[w_orig]
                m.add_WS(w_error)
            except KeyError:
                m = Matrix(w_orig)
                cf_info[w_orig] = m
                m.add_WS(w_error)
                #NOSUGG they're (should be: their)
        m = re.match( "NOSUGG (.*?) \(should be: (.*?)\)", line )
        if m:
            #print line
            w_orig  = m.group(2)
            w_error = m.group(1)
            #print w_orig, w_error
            try:
                m = cf_info[w_orig]
                m.add_NS()
            except KeyError:
                m = Matrix(w_orig)
                cf_info[w_orig] = m
                m.add_NS()

'''
Latex header
'''
print '''
\\begin{table}[h!]
\\caption{'''+filename.replace('_', '\\_')+'''}
\\centering
\\vspace{\\baselineskip}
%\\fit{%
\\begin{tabular}{@{} l r r r r r l r r r r r @{}} %
\\toprule
 & total & \\gs{} & \\ws{} & \\ns{} & \\bs{} &  & total & \\gs{} & \\ws{} & \\ns{} & \\bs{}\\\\
\midrule'''

total = 0
'''
for m in cf_info:
    #print cf_info[m].to_str()
    print cf_info[m].to_latex()
    total += cf_info[m].total()
print total
'''
for cf_set in confusibles:
    l = len(cf_set)
    out = ""
    for cf in cf_set:
        if cf in cf_info:
            out += cf_info[cf].to_latex() + " & "
        else:
            out += cf + " & \\tnum{0} & \\tnum{0} & \\tnum{0} & \\tnum{0} & \\tnum{0} & "
    if l == 2:
        print out[:-2],"\\\\"
    else:
        print "%",out[:-2],"\\\\"
        
print '''
\\bottomrule
\\end{tabular}%}%endfit
\\label{tab:LABEL}
\\end{table}
'''

# triple word sets

print '''
\\begin{table}[h!]
\\caption{'''+filename.replace('_', '\\_')+'''}
\\centering
\\vspace{\\baselineskip}
%\\fit{%
\\begin{tabular}{@{} l r r r r r l r r r r r l r r r r r @{}} %
\\toprule
 & total & \\gs{} & \\ws{} & \\ns{} & \\bs{} &  & total & \\gs{} & \\ws{} & \\ns{} & \\bs{} & & total & \\gs{} & \\ws{} & \\ns{} & \\bs{}\\\\
\midrule'''

for cf_set in confusibles:
    l = len(cf_set)
    out = ""
    for cf in cf_set:
        if cf in cf_info:
            out += cf_info[cf].to_latex() + " & "
        else:
            out += cf + " & \\tnum{0} & \\tnum{0} & \\tnum{0} & \\tnum{0} & \\tnum{0} & "
    if l == 3:
        print out[:-2],"\\\\"
    else:
        pass #print "%",out[:-2],"\\\\"
        
print '''
\\bottomrule
\\end{tabular}%}%endfit
\\label{tab:LABEL}
\\end{table}
'''
