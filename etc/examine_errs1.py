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

Update 2014-01-06 option to show acc/pre/recall instead, choice is not implemented yet

2014-04-06
Example:
python examine_errs1.py -f utexas.10e6.dt3.t1e5d.cf350.l2r2_ML00000.sc.errs1 -c goldingroth3.txt

2014-04-08
stats on all confusibels, not only the errored ones.
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
        self.outputformat = 1 #0=gs/ws, 1=acc/rec
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
    def get_stats(self, show_bs):
        """
        my $TP = $good_sugg;
        my $FN = $wrong_sugg + $no_sugg;
        my $FP = $bad_sugg;
        my $TN = $clines - $TP - $FN - $FP; #Niks gedaan als niks moest

        my $RCL = 0;
        if ( $TP+$FN > 0 ) {
          $RCL = ($TP*100)/($TP+$FN);           #TPR
        }
        my $ACC = (100*($TP+$TN))/($clines);

        my $PRC = 0;
        if ( $TP+$FP > 0 ) {
          $PRC = (100*$TP)/($TP+$FP);           #PPV
        }
        my $FPR = 0;
        if ( $FP+$TN > 0 ) {
          $FPR = (100*$FP)/($FP+$TN);
        }
        my $F1S = (100*2*$TP)/((2*$TP)+$FP+$FN);
        """
        if self.outputformat == 1:
            TP = self.GS
            FN = self.WS + self.NS
            FP = self.BS
            ALL = self.GS+self.WS+self.NS+self.BS #self.total() + self.BS
            TN = ALL - TP - FN - FP
            RCL = 0
            ACC = 0
            if TP + FN > 0:
                RCL = (TP*100.0)/(TP+FN)
            if ALL > 0:
                ACC = (100.0*(TP+TN))/(ALL)
            PRC = 0
            if TP+FP > 0:
                PRC = (100.0*TP)/(TP+FP)
            return (self.total(), ACC, PRC, RCL)
        else:
            if show_bs:
                return (self.total(), self.GS, self.WS, self.NS, self.BS)
            else:
                return (self.total(), self.GS, self.WS, self.NS)
        
    def to_str(self, show_bs):
        if show_bs:
            ans = self.confusible+": TOT:"+str(self.total())+" GS:"+str(self.GS)+" WS:"+str(self.WS)+" NS:"+str(self.NS)+" BS:"+str(self.BS)
        else:
            ans = self.confusible+": TOT:"+str(self.total())+" GS:"+str(self.GS)+" WS:"+str(self.WS)+" NS:"+str(self.NS)
        return ans
    def to_latex(self, show_bs):
        if self.outputformat == 1:
            TP = self.GS
            FN = self.WS + self.NS
            FP = self.BS
            ALL = self.GS+self.WS+self.NS+self.BS #self.total() + self.BS
            TN = ALL - TP - FN - FP
            RCL = 0
            ACC = 0
            if TP + FN > 0:
                RCL = (TP*100.0)/(TP+FN)
            if ALL > 0:
                ACC = (100.0*(TP+TN))/(ALL)
            PRC = 0
            if TP+FP > 0:
                PRC = (100.0*TP)/(TP+FP) #"{0:6.2f}".format(2.34567)
            ans = self.confusible+" & \\tnum{"+str(self.total())+"} & \\tnum{"+"{0:.2f}".format(ACC)+"} & \\tnum{"+"{0:.2f}".format(PRC)+"} & \\tnum{"+"{0:.2f}".format(RCL)+"}"
        else:
            if show_bs:
                ans = self.confusible+" & \\tnum{"+str(self.total())+"} & \\tnum{"+str(self.GS)+"} & \\tnum{"+str(self.WS)+"} & \\tnum{"+str(self.NS)+"} & \\tnum{"+str(self.BS)+"}"
            else:
                ans = self.confusible+" & \\tnum{"+str(self.total())+"} & \\tnum{"+str(self.GS)+"} & \\tnum{"+str(self.WS)+"} & \\tnum{"+str(self.NS)+"}"
        return ans
    
all_files = None
cfile = None
show_bs = False
text_only = False
outputformat = 1 #shows acc/pre/rec instead of gs/ws/...

try:
    opts, args = getopt.getopt(sys.argv[1:], "bc:f:o:t", ["file="])
except getopt.GetoptError, err:
    #print str(err)
    sys.exit(2)
for o, a in opts:
    if o in ("-f", "--file"):
        all_files = [ a ]
    elif o in ("-c", "--cfile"):
        cfile = a 
    elif o in ("-b", "--showbs"):
        show_bs = True
    elif o in ("-o", "--outputformat"):
        outputformat = int(a)
    elif o in ("-t", "--textonly"):
        text_only = True
    else:
        assert False, "unhandled option"

        
if all_files == None:
    print "Need a .errs1 file (-f parameter)."
    sys.exit(1)

if cfile == None:
    print "Need a confusible file (-c parameter)."
    sys.exit(1)
    
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

#print confusibles
        
for filename in all_files:
    with open(filename, 'r') as f:
        file_lines = f.readlines()
    w_error = None
    w_orig = None
    for line in file_lines:
        matched = False
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
        m = re.match( "BADSUGG (.*?) \-> (.*?) \(should be: (.*?)\)", line )
        if m:
            #print line
            w_orig  = m.group(3)
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


total = 0
for m in cf_info:
    print cf_info[m].to_str(show_bs)
    total += cf_info[m].total()
print total

if outputformat == 1:
    print '{0:<10} {1:>3} {2:>6} {3:>6} {4:>6}   {0:<10} {1:>3} {2:>6} {3:>6} {4:>6}'.format("C", "T", "acc", "pre", "rec" )
else:
    if show_bs:
        print '{0:<10} {1:>3} {2:>3} {3:>3} {4:>3} {5:>3}   {0:<10} {1:>3} {2:>3} {3:>3} {4:>3} {5:>3}'.format( "C", "T", "gs", "ws", "ns", "bs" )
    else:
        print '{0:<10} {1:>3} {2:>3} {3:>3} {4:>3}   {0:<10} {1:>3} {2:>3} {3:>3} {4:>3}'.format("C", "T", "gs", "ws", "ns" )
for cf_set in confusibles:
    l = len(cf_set)
    out = ""
    for cf in cf_set:
        if cf in cf_info:
            stats = cf_info[cf].get_stats(show_bs)
            if outputformat == 1:
                out += '{0:<10} {1:3n} {2:6.2f} {3:6.2f} {4:6.2f}   '.format( cf, *stats )
            else:
                if show_bs:
                    out += '{0:<10} {1:3n} {2:3n} {3:3n} {4:3n} {5:3n}   '.format( cf, *stats )
                else:
                    out += '{0:<10} {1:3n} {2:3n} {3:3n} {4:3n}   '.format( cf, *stats )
        else: #cf not in cf_info
            if outputformat == 1:
                out += '{0:<10} {1:3n} {2:3n} {3:3n} {4:3n}   '.format( cf, 0, 0, 0, 0 )
            else:
                if show_bs:
                    out += '{0:<10} {1:3n} {2:3n} {3:3n} {4:3n} {5:3n}   '.format(cf, 0, 0, 0, 0, 0)
                else:
                    out += '{0:<10} {1:3n} {2:6.2f} {3:6.2f} {4:6.2f}   '.format( cf, 0, 0, 0, 0 )
    print out

if text_only:
    sys.exit(0)
        
'''
Latex header
'''
print '''
\\begin{table}[h!]
\\caption{'''+filename.replace('_', '\\_')+'''}
\\centering
\\vspace{\\baselineskip}
%\\fit{%'''
if outputformat == 1:
    print "\\begin{tabular}{@{} l r r r r l r r r r @{}} %\n"
    print "\\toprule\n"
    print " & total & \\acc{} & \\pre{} & \\rec{} &  & total & \\acc{} & \\pre{} & \\rec{} \\\\\n"
else:
    if show_bs:
        print "\\begin{tabular}{@{} l r r r r r l r r r r r @{}} %\n"
        print "\\toprule\n"
        print " & total & \\gs{} & \\ws{} & \\ns{} & \\bs{} &  & total & \\gs{} & \\ws{} & \\ns{} & \\bs{}\\\\\n"
    else:
        print "\\begin{tabular}{@{} l r r r r l r r r r @{}} %\n"
        print "\\toprule\n"
        print " & total & \\gs{} & \\ws{} & \\ns{} &  & total & \\gs{} & \\ws{} & \\ns{} \\\\\n"
print "\\midrule\n"

for cf_set in confusibles:
    l = len(cf_set)
    out = ""
    for cf in cf_set:
        if cf in cf_info:
            out += cf_info[cf].to_latex(show_bs) + " & "
        else:
            #here is the error
            if show_bs:
                out += cf + " & \\tnum{0} & \\tnum{0} & \\tnum{0} & \\tnum{0} & \\tnum{0} & "
            else:
                out += cf + " & \\tnum{0} & \\tnum{0} & \\tnum{0} & \\tnum{0} & "
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
%\\fit{%'''
if show_bs:
    print "\\begin{tabular}{@{} l r r r r r l r r r r r l r r r r r @{}} %\n"
    print "\\toprule\n"
    print " & total & \\gs{} & \\ws{} & \\ns{} & \\bs{} &  & total & \\gs{} & \\ws{} & \\ns{} & \\bs{} & & total & \\gs{} & \\ws{} & \\ns{} & \\bs{}\\\\\n"
else:
    print "\\begin{tabular}{@{} l r r r r l r r r r l r r r r @{}} %\n"
    print "\\toprule\n"
    print " & total & \\gs{} & \\ws{} & \\ns{} &  & total & \\gs{} & \\ws{} & \\ns{} & & total & \\gs{} & \\ws{} & \\ns{} \\\\\n"
print "\\midrule\n"

for cf_set in confusibles:
    l = len(cf_set)
    out = ""
    for cf in cf_set:
        if cf in cf_info:
            out += cf_info[cf].to_latex(show_bs) + " & "
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
