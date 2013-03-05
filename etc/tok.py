#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
import glob
import os
import re
import sys
import codecs

file = "foo.bla"
if len(sys.argv) > 1:
  file = sys.argv[1]

# ----

f = codecs.open(file, "r", "utf-8")
#lines = f.readlines()
#f.close()
#lines = [line[:-1] for line in lines]

for line in f:
  # need special case for 'word' (single quotes, versus "one's")
  # he said 'no' <- if start AND end == "'"
  # he said 'no you fool' <- lastiger... loop over string, see if one
  # start' and one end' ?
  #
  tok =  re.findall(r"[\w']+|[.,!?;\"]", line, re.UNICODE)
  print (' '.join(tok)).encode("utf-8")
