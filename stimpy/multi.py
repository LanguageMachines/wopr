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

#s1=TServers("localhost",2000)
#s1.topn=1
##dflt0=Classifier("dflt0", 2, 0, s1)
#dflt1=Classifier("dflt1", 3, 0, s1)
#allc=[ dflt0, dflt1 ]

s1=TServers("localhost",2000)
#austen.train.l2r0.c0000:to too two
ts0000=Classifier("ts0000", 2, 0, s1)
ts0000.set_triggers(["to","too","two" ]);
#austen.train.l2r0.c0001:than then
ts0001=Classifier("ts0001", 2, 0, s1)
ts0001.set_triggers(["than","then" ]);
#austen.train.l2r0.c0002:their there
ts0002=Classifier("ts0002", 2, 0, s1)
ts0002.set_triggers(["their","there" ]);
dflt0=Classifier("dflt0", 2, 0, s1)
allc=[ ts0000, ts0001, ts0002, dflt0 ]

if lexfile != None:
    s1.read_lex(lexfile)

if afile != None:
    res = [s1.classify( line[:-1] ) for line in open(afile)]
    for rr in res:
        #for r in rr:
        print ans_str(rr)

if instance != None:
    c = s1.get_trigger(instance)
    if c:
        c.classify_i(instance)
        if not c.error:
            print c.name,":",ans_str(c.ans)
    print "----"
    for c in allc:
        #print c
        c.classify_i(instance)
        if not c.error:
            print c.name,":",ans_str(c.ans)

if sentence != None:
    all_res = dict()
    for c in allc:
        res = c.classify_s(sentence)
        all_res[c.name] = res
        print c.name,":"
        for r in res:
            pass
            print ans_str(r)
    '''
    r0 = all_res["dflt0"]
    r1 = all_res["dflt1"]
    dl = [ (ans_str(a[0]),ans_str(a[1])) for a in zip(r0,r1) ]
    for d in dl:
        print d[0]
        print d[1]
        print
    '''
        
