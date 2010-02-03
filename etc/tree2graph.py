#!/usr/bin/env python
# -*- Mode: Python -*-

# Note:
# parsing is far from fool proof and may not terminate
# with an incomplete or ill-formed instance base file

# PJB: needs hashed file
# PJB: depth added, and redid dump()
# PJB: removed: replace('"', '\\"')
# PJB: output() toegevoegd
# PJB: output2() toegevoegd
# PJB: tikz toegevoegd
# PJB: redid output, include sum and log

# ----
# Trein 14/2/08: order of features?? (-TDO)
#   arc[1].output(pat + ' ' + ftr, depth-1)
# of
#   arc[1].output(ftr + ' ' + pat, depth-1)
# /Users/pberck/prog/UvT/Timbl6/src/Timbl -f num3a -I num3a.a1a -a1 +D
# python ./tree2graph num3a.a1a
#   Die ftr=pat een array van maken, en de juiste plaatsen opvullen
#   ipv. concatenatie.
#
# Or without class, and in right order See tree4, last graph.
# Default dummy catery in the endin the whole dataset.
# ----

import string, sys, math

class Node:
   def __init__(self, default_class):
       self.default_class = default_class
       self.class_dist = []
       self.arcs = []
   def append_distribution(self, clas, freq):
       dist = (clas, freq)
       self.class_dist.append(dist)
       #print dist
   def append_child(self, value, node):
       arc = (value, node)
       self.arcs.append(arc)
   def dump(self, indent = 0, depth = 3):
       cls = classes[self.default_class - 1]
       print indent * ' ', 'Default class:', cls #was self.default_class
       for dist in self.class_dist:
          cls = classes[dist[0] - 1]
          print indent * ' ', 'dist:', cls, dist[1] #cls was dist[0]
       if depth != 0:
          for arc in self.arcs:
             ftr = features[arc[0] - 1]
             print  indent * ' ', 'Arc:', ftr #was arc[0]
             arc[1].dump(indent + 4, depth-1)
   def output(self, pat = "", depth = 3):
       cls = classes[self.default_class - 1]
       sum = 0
       for dist in self.class_dist:
          sum = sum + dist[1]
       #print "sum", sum
       for dist in self.class_dist:
          cls = classes[dist[0] - 1]
          #print pat, cls, dist[1], float(dist[1]/float(sum))
          print math.log(float(dist[1]/float(sum)), 10), pat, cls
       if depth != 0:
          for arc in self.arcs:
             ftr = features[arc[0] - 1]
             arc[1].output(pat + ' ' + ftr, depth-1)
             #arc[1].output(ftr + ' ' + pat, depth-1)
   def output2(self, path = "", depth = 3):
      children = len( self.arcs )
      # check depth?
      if depth != 0 and children > 0:
         for arc in self.arcs:
            ftr = features[arc[0] - 1]
            #print depth, ftr
            arc[1].output2(ftr+path, depth-1)
      else: #children==0
         cls = classes[self.default_class - 1]
         #print "bottom", cls
         #print "path", path
         print path+cls
         sum = 0
         for dist in self.class_dist:
            cls = classes[dist[0] - 1]
            sum = sum + dist[1]
            print cls, dist[1] #hier dists etc optellen, probs uitrekenen
         print "sum", sum
   def tikz(self, indent="", lbl=""):
      cls = classes[self.default_class - 1]
      distlist = ""
      #print "dist",self.class_dist
      for dist in self.class_dist:
         cls = classes[dist[0] - 1]
         distlist = distlist+cls+","+str(dist[1])+" "
      distlist = distlist + ""
      if 3==4: # distlist == "":
         print indent+"child {node {"+cls+"}%"
      else:
         print indent+"child {node {"+distlist+"}%"
         #print indent+"child {node[label=below:"+distlist+"] {"+cls+"}%"
      children = len( self.arcs )
      if children > 0:
         for arc in self.arcs:
            ftr = features[arc[0] - 1]
            arc[1].tikz(indent+" ", ftr)
      print indent+"edge from parent node[left] {\\textsl{"+lbl+"}} }"

def parse_node(stream):
   # skip openening parenthesis
   stream.read(1)
   # read default class
   default_class = ''
   while 1:
      c = stream.read(1)
      if not c in string.digits:
         break
      default_class = default_class + c
   node = Node(int(default_class))
   if c == '{':
      # parse list with distributions
      # (i.e. class & frequency pairs) 
      while 1:
         c = stream.read(1)
         if c == '}':
            break
         if c == ' ':
            continue
         else:
            # read class
            clas = c
            while 1:
               c = stream.read(1)
               if c == ' ':
                  break
               clas = clas + c
            # read frequency
            freq = ''
            while 1:
               c = stream.read(1)
               if c in ', ':
                  break
               freq = freq + c
            node.append_distribution(int(clas), int(freq))
            #print "-->",clas,freq
      c = stream.read(1)
   if c == '[':
      # parse list of children
      while 1:
         c = stream.read(1)
         if c == ']':
            # skip newline
            # and node's closing parenthesis
            stream.read(2)
            break
         if c == ',':
            continue
         value = c
         # read arc's feature value 
         while 1:
            c = stream.read(1)
            if c == '(':
               break
            value = value + c
         # move back one position to the opening parenthesis
         # of the child node
         stream.seek(stream.tell() - 1)
         child = parse_node(stream)
         node.append_child(int(value), child)
   # skip newline after this node's closing parenthesis
   stream.read(1)
   return node        
            
    
def parse_hash_table(stream):
   # straight forward,
   # but unsafe if file format changes slightly
   classes = []
   features = []
   lst = []
   while 1:
      l = stream.readline()[:-1]
      if not l:
         break
      if l[0] == '#':
         continue
      if l == 'Classes':
         lst = classes
      elif l == 'Features':
         lst = features
      else:
         lst.append(string.split(l, '\t')[1])
   return classes, features
            
            
def generate_dot_graph(tree, classes, features):
   print 'digraph g {'
   generate_dot_node(tree, classes, features, 'n')
   print '}'


def generate_dot_node(node, classes, features, node_id):
   print '%s [label="%s"];' % (node_id,
                               # escape double quote in label
                               classes[node.default_class - 1].replace('"', '\\"'))
   for arc_nr in range(len(node.arcs)):
      child_id = node_id + '_' + str(arc_nr)
      generate_dot_node(node.arcs[arc_nr][1], classes, features, child_id)
      print '%s -> %s [label="%s"];' % (node_id.replace('"', '\\"'),
                                        child_id,
                                        # escape double quote in label
                                        features[node.arcs[arc_nr][0] - 1].replace('"', '\\"'))
      
   
if __name__ == '__main__':
   if ( len(sys.argv) == 1 or
        sys.argv[1] == '-h' or
        sys.argv[1] == '--help'):
      print """
      Usage: tree2graph instance-base-fn

      Maps Timbl instance base (as produced with -I option)
      to an input file for the graph layout program 'dot'. 

      $Id$
      """
      sys.exit()
   f = open(sys.argv[1])
   classes, features = parse_hash_table(f)
   tree = parse_node(f)
   tree.dump(0,10)
   tree.output( "", 3 )
   print
   print "\\begin{tikzpicture}"
   print "\\node {root}"
   tree.tikz()
   print ";"
   print "\\end{tikzpicture}"
   #generate_dot_graph(tree, classes, features)
   f.close()
