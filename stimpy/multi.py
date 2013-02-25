#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# Simple TIMbleserver PYthon Interface
#
from stimpy import *
import getopt
import sys

host = "localhost"
port = 2000
lexfile = None
afile = None
instance = None
sentence = None
topn = 3
debug = False

try:
    opts, args = getopt.getopt(sys.argv[1:], "f:i:l:L:h:n:p:r:s:", ["lc="])
except getopt.GetoptError, err:
    # print help information and exit:
    print str(err) # will print something like "option -a not recognized"
    #usage()
    sys.exit(2)
for o, a in opts:
    if o in ("-f"):
        afile = a
    elif o in ("-i"):
        instance = a
    elif o in ("-l", "--lc"):
        lc = a
    elif o in ("-L"):
        lexfile = a
    elif o in ("-h"):
        host = a
    elif o in ("-n"):
        topn = int(a)
    elif o in ("-p"):
        port = a
    elif o in ("-r"):
        rc = a
    elif o in ("-s"):
        sentence = a
    else:
        assert False, "unhandled option"
"""
all_files = []
if dirmode:
    # Find all
    #
    test     = re.compile(test_str, re.IGNORECASE)
    files = os.listdir( the_dir )
    files = filter(test.search, files)    
    for a_file in files:
        all_files.append( the_dir + a_file)
else:
    all_files.append( a_file )
"""

# --

'''s1=TServers("localhost",2000)
ts0101=Classifier("ts0101", 2, 1, s1)
dflt0=Classifier("dflt0", 2, 1, s1)
ts0201=Classifier("ts0201", 2, 2, s1)
dflt1=Classifier("dflt1", 2, 2, s1)
ts0301=Classifier("ts0301", 3, 0, s1)
dflt2=Classifier("dflt2", 3, 0, s1)
allc=[ ts0101, dflt0, ts0201, dflt1, ts0301, dflt2 ]

s1=TServers("localhost",2000)
ts0000=Classifier("ts0000", 2, 0, s1)
ts0001=Classifier("ts0001", 2, 0, s1)
ts0002=Classifier("ts0002", 2, 0, s1)
ts0003=Classifier("ts0003", 2, 0, s1)
ts0004=Classifier("ts0004", 2, 0, s1)
ts0005=Classifier("ts0005", 2, 0, s1)
ts0006=Classifier("ts0006", 2, 0, s1)
ts0009=Classifier("ts0009", 2, 0, s1)
ts0012=Classifier("ts0012", 2, 0, s1)
ts0016=Classifier("ts0016", 2, 0, s1)
ts0017=Classifier("ts0017", 2, 0, s1)
ts0018=Classifier("ts0018", 2, 0, s1)
ts0020=Classifier("ts0020", 2, 0, s1)
ts0021=Classifier("ts0021", 2, 0, s1)
ts0024=Classifier("ts0024", 2, 0, s1)
ts0025=Classifier("ts0025", 2, 0, s1)
ts0026=Classifier("ts0026", 2, 0, s1)
ts0028=Classifier("ts0028", 2, 0, s1)
ts0029=Classifier("ts0029", 2, 0, s1)
ts0031=Classifier("ts0031", 2, 0, s1)
ts0032=Classifier("ts0032", 2, 0, s1)
ts0033=Classifier("ts0033", 2, 0, s1)
ts0036=Classifier("ts0036", 2, 0, s1)
dflt0=Classifier("dflt0", 2, 0, s1)
allc=[ ts0000, ts0001, ts0002, ts0003, ts0004, ts0005, ts0006, ts0009, ts0012, ts0016, ts0017, ts0018, ts0020, ts0021, ts0024, ts0025, ts0026, ts0028, ts0029, ts0031, ts0032, ts0033, ts0036, dflt0 ]
'''

s1=TServers("localhost",2000)
s1.topn=1
dflt0=Classifier("dflt0", 2, 0, s1)
dflt1=Classifier("dflt1", 3, 0, s1)
allc=[ dflt0, dflt1 ]

# Classifiers made with wopr_exp1.script
#ts0 = Classifier("ts0", 2, 0, s1)
#ts0.to = 1
#cs1 = [  Classifier("dflt", 2, 0, s1), ts0 ]

if lexfile != None:
    s1.read_lex(lexfile)

if afile != None:
    res = [s1.classify( line[:-1] ) for line in open(afile)]
    for rr in res:
        #for r in rr:
        print ans_str(rr)

if instance != None:
    for c in allc:
        print c
        c.classify_i(instance)
        print c.name,":",ans_str(c.ans)

if sentence != None:
    all_res = dict()
    for c in allc:
        res = c.classify_s(sentence)
        all_res[c.name] = res
        print c.name,":"
        for r in res:
            pass
            #print ans_str(r)
    r0 = all_res["dflt0"]
    r1 = all_res["dflt1"]
    dl = [ (ans_str(a[0]),ans_str(a[1])) for a in zip(r0,r1) ]
    #print dl
    for d in dl:
        print d[0]
        print d[1]
        print

        
