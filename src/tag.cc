// ---------------------------------------------------------------------------
// $Id$
// ---------------------------------------------------------------------------

/*****************************************************************************
 * Copyright 2008-2014 Peter Berck                                           *
 *                                                                           *
 * This file is part of wopr.                                                *
 *                                                                           *
 * wopr is free software; you can redistribute it and/or modify it           *
 * under the terms of the GNU General Public License as published by the     *
 * Free Software Foundation; either version 2 of the License, or (at your    *
 * option) any later version.                                                *
 *                                                                           *
 * wopr is distributed in the hope that it will be useful, but WITHOUT       *
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or     *
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License      *
 * for more details.                                                         *
 *                                                                           *
 * You should have received a copy of the GNU General Public License         *
 * along with wpred; if not, write to the Free Software Foundation,          *
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA               *
 *****************************************************************************/

/*

*/

// ---------------------------------------------------------------------------
//  Includes.
// ---------------------------------------------------------------------------

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <map>
#include <vector>
#include <algorithm>
#include <iterator>

#include <unistd.h>
#include <stdio.h>
#include <math.h>

#include <sys/msg.h>

#include "wopr/qlog.h"
#include "wopr/util.h"
#include "wopr/Config.h"
#include "wopr/runrunrun.h"
#include "wopr/watcher.h"
#include "wopr/tag.h"

// ---------------------------------------------------------------------------
//  Code.
// ---------------------------------------------------------------------------

Arc::Arc( int c, Node* n ) {
  node = n;
  label = c;
}

int Arc::get_label() {
  return label;
}

Node* Arc::get_node() {
  return node;
}

Node::Node( int n ) {
  classhash = n;
  //std::cout << "Node(" << n << ")\n";
}

void Node::set_defaultclass( int c ) {
  classhash = c;
}

void Node::add_dist( int c, int f ) {
  //std::cout << "Dist(" << c << "," << f << ")\n";
  dist.push_back(c);
  dist.push_back(f);
}

void Node::add_arc( int c, Node* n ) { // this is add_child really
  //std::cout << "Arc(" << c << ")\n";
  Arc* arc = new Arc( c, n );
  arcs.push_back( arc );
}

std::string Node::info() {
  return "Node: "+to_str(classhash)+" "+get_dist();
}

void Node::dump() {
  std::string d = get_dist();
  std::cout << d << ":["<< arcs.size() << "]\n";
  for ( size_t i = 0; i < arcs.size(); i++ ) {
    Arc* a = arcs[i];
    std::cout << a->get_label() << std::endl;
    Node* n = a->get_node();
    n->dump();
  }
}

// Arpa dump.
// Need to fill the 'holes', that is, make patterns with default
// categories on nodes.
//
void Node::dump( std::map<int, std::string>& classes,
		 std::map<int, std::string>& features, std::string pat,
		 int d, std::ofstream& os ) {

  // pattern we generate is class from dist with pat prepended.
  // log10 calculated from local apriori probs.
  // We need a place to insert our smoothed values.
  //
  int sum = 0;
  for ( size_t i = 0; i < dist.size(); i += 2 ) {
    sum += (int)dist[i+1];
  }

  if ( d == 0 ) { // d is depth, we are at the 'end' here.

    //os << "Dflt: " << classes[ classhash ] << std::endl; //PJB ??

    for ( size_t i = 0; i < dist.size(); i += 2 ) {
      std::string the_class = classes[ dist[i] ];
      int class_freq = (int)dist[i+1]; // INSERT smoothed value here <---
      float class_prob = (float)(class_freq / (float)sum);
      //classprob 1 gives 0 here.
      //if ( class_prob != 1.0 ) {
      os << log10(class_prob) << " " << pat << the_class; // << " 0.01";
      os << std::endl;
      //}
    }
  }

  for ( size_t i = 0; i < arcs.size(); i++ ) {
    Arc* a = arcs[i];
    //std::cout << features[ a->get_label() ] << std::endl;
    Node* n = a->get_node();
    std::string new_pat = features[ a->get_label() ] + " " + pat;
    //std::string new_pat = pat + " " + features[ a->get_label() ];
    n->dump(classes, features, new_pat, d-1, os );
  }
}

// Same as above, but smoothes the frequencies using smooth
// smooth: count -> count*
//
void Node::dump( std::map<int, std::string>& classes,
		 std::map<int, std::string>& features, std::string pat,
		 int d, std::ofstream& os,
		 std::map<int, double>& smooth ) {

  // pattern we generate is class from dist with pat prepended.
  // log10 calculated from local apriori probs.
  // We need a place to insert our smoothed values.
  //
  double sum = 0.0;
  double o_sum = 0.0;
  std::vector<double> sdist(dist.size()); // dist: [c1, f1, ...]
  smooth_dist( smooth, sdist );
  for ( size_t i = 0; i < sdist.size(); i += 2 ) {
    sum += (double)sdist[i+1];
    o_sum += (double)dist[i+1];
  }

  if ( d == 0 ) { // d is depth, we are at the 'end' here.
    // Generate a pattern for each class in the dist.
    for ( size_t i = 0; i < sdist.size(); i += 2 ) {
      std::string the_class = classes[ (int)sdist[i] ];
      double class_freq = (double)sdist[i+1];
      if ( (int)class_freq == 0 ) {
	class_freq = (double)dist[i+1];
      }
      double class_prob = (double)(class_freq / (double)sum);
      // check if class_prob is 0, dan neem p0 from smoothin
      os << log10(class_prob) << " " << pat << the_class << std::endl;
    }
  }

  for ( size_t i = 0; i < arcs.size(); i++ ) {
    Arc* a = arcs[i];
    Node* n = a->get_node();
    std::string new_pat = features[ a->get_label() ] + " " + pat;
    n->dump(classes, features, new_pat, d-1, os, smooth );
  }
}

std::string Node::get_dist() {
  std::string res;
  for ( size_t i = 0; i < dist.size(); i++ ) {
    res += to_str((int)dist[i])+" ";
  }
  return res;
}

// Can we assume all values are present?
//
/*
ffreqs opbouwen als we de boom parsen, dan smoothen. Mixen we dan teveel?

Of tellen als we de output doen, ffreqs & smoothing per lengte
van patroon. Nee.
*/
void Node::smooth_dist( std::map<int, double>& smooth, // count -> count*
			std::vector<double>& res ) {
  // dist: [ class1, f1, class2, f2,..., classn, fn ]
  for ( size_t i = 0; i < dist.size(); i +=2 ) {
    int     f = (int)dist[i+1];
    double sf = (double)smooth[f];
    res[i] = dist[i];
    res[i+1] = sf;
  }
}

// ---

int read_digits( std::ifstream& f ) {
  char chr;
  std::string res;

  while ( f.get( chr ) && std::isdigit( chr )) {
    res += chr;
  }
  f.putback( chr );

  return my_stoi( res );
}

void read_dist( Node* n, std::ifstream& f, int d, std::vector<int>& counts ) {
  char chr;
  std::string res;

  f.get( chr ); // skip space
  int number;
  int freq;
  int count = 0;
  bool readdist = true;
  while ( readdist ) {
    number = read_digits( f );
    f.get( chr ); // skip space
    freq = read_digits( f );
    n->add_dist( number, freq ); // Insert smoothed value already here?
    // that would allow us to save back a smoothed ibase.
    // error....Timbl wants integers.
    ++count;

    f.get( chr );
    if ( chr != ',' ) {
      readdist = false;
      counts[d+1] += count;
      break;
    }
    f.get( chr );
  }
  f.get( chr ); // '}'
}

// Start on a digit.
// Can we count n-grams here?
// We don't have to build deeper than some max-depth? We still need to parse.
//
Node* read_node( std::ifstream& f, int depth, std::vector<int>& counts ) {

  int number = read_digits( f ); // default class
  char chr;

  Node* a_node = new Node( number );

  // What's next, { or ...
  //
  bool working = true;

  while ( working ) {

    if ( ! f.get( chr ) ) {
      working = false;
      return a_node; // was break;
    }

    if ( chr == ' ' ) {
      continue;
    }

    if ( chr == ')' ) {
      f.get( chr ); // eat CR
      working = false; // could be a comma after. could be more children.
      break;
    }

    if ( chr == ']' ) {
      f.get( chr ); // eat CR
      continue;
    }

    if ( chr == '{' ) {// { 1 3,
      read_dist( a_node, f, depth, counts );
      continue;
    }

    if ( chr == '[' ) { // children, arcs.
      number = read_digits( f ); // label on arc
      f.get( chr ); // '('
      Node* c_node = read_node( f, depth+1, counts ); // go down and read
      // add to this node as arc.
      a_node->add_arc( number, c_node );
      continue;
    }

    if ( chr == ',' ) {
      number = read_digits( f ); // label on arc
      f.get( chr ); // '(' ?
      Node* c_node = read_node( f, depth+1, counts ); // go down and read
      // add to this node as arc.
      a_node->add_arc( number, c_node );
    }

  }

  return a_node;
}


// ----

// Timbl Arpa generator
//
int tag( Logfile& l, Config& c ) {

  const std::string& ibase_filename = c.get_value( "ibasefile" );
  const int n = my_stoi( c.get_value( "n", "3" ));
  const std::string& counts_filename = c.get_value( "counts", "" );
  const std::string arpa_filename = ibase_filename + ".arpa";

  l.inc_prefix();
  l.log( "ibase:  " + ibase_filename );
  l.log( "counts: " + counts_filename );
  l.log( "n:      " + to_str(n) );
  l.log( "Output: " + arpa_filename );
  l.dec_prefix();

  // Parse first bit, the hashed filenames. Non-hashed version also?

  /*
    node has: default dist (not really used)
              class dist (class + value)
              arc list (value + node )
  */

  // Counts.
  //
  std::map<int,double> ffreqs;
  if ( counts_filename != "" ) {
    l.log( "Reading counts." );
    std::ifstream file_counts( counts_filename.c_str() );
    if ( ! file_counts ) {
      l.log( "ERROR: cannot load file." );
      return -1;
    }
    int count;
    int Nc0;
    double c_star;
    int numcounts = 0;
    //
    // Format of .cnt: 4 764 3.6322
    //                 | |   |
    //                 ^frequency
    //                   ^number with this frequency
    //                       ^adjusted frequency
    //
    while( file_counts >> count >> Nc0 >> c_star ) {
      ffreqs[count] = (double)c_star; // maps frequency to adjusted frequency
      ++numcounts;
    }
    file_counts.close();
    l.log( "Read counts." );
  }

  // Load ibase
  //
  l.log( "Reading ibase." );
  std::ifstream file_ibase( ibase_filename.c_str() );
  if ( ! file_ibase ) {
    l.log( "ERROR: cannot load file." );
    return -1;
  }
  std::string a_line;
  int mode = 0;
  std::string name, hashname;
  std::map<int, std::string> classes;
  std::map<int, std::string> features;
  std::vector<int> counts(88, 0); //88 is arbitrary limit...
  Node *top_node = 0;
  long here; // pos_type?
  while( std::getline( file_ibase, a_line )) {
    if ( a_line == "Classes" ) {
      mode = 1;
      l.log( "Reading classes" );
    }
    if ( mode == 1 ) {
      file_ibase >> hashname;
      if ( hashname == "Features" ) {
	mode = 2;
	l.log( "Reading features" );
	continue;
      }
      file_ibase >> name;
      //l.log( hashname + " " + name );
      classes[my_stoi(hashname)] = name;
    }
    if ( mode == 2 ) {
      // parse and store features
      here = file_ibase.tellg();
      file_ibase >> hashname;
      if ( hashname[0] == '(' ) {
	file_ibase.seekg( here ); // rewind fileptr.
	mode = 3;
	l.log( "Reading tree" );
	continue;
      }
      file_ibase >> name;
      //l.log( hashname + " " + name );
      features[my_stoi(hashname)] = name;
    }

    if ( mode == 3 ) {
      char chr;
      file_ibase.get( chr ); // skip first '('
      top_node = read_node( file_ibase, 0, counts );
      break;
    }
  }
  file_ibase.close();

  l.log( "Writing ARPA" );
  std::ofstream file_arpa( arpa_filename.c_str(), std::ios::out );
  if ( ! file_arpa ) {
    l.log( "ERROR: cannot write file." );
    return -1;
  }

  // Generate the ARPA format.
  // We need to add the back off weight in the ARPA file...
  //
  file_arpa << "\\" << "data\\" << std::endl;

  for ( int d = 0; d < n; d++ ) {
    file_arpa << "ngram " << d+1 << "=" << counts[d+1] << std::endl;
  }

  for ( int d = 0; d < n; d++ ) {
    file_arpa << std::endl << "\\" << d+1 << "-grams:" << std::endl;
    if ( counts_filename == "" ) {
      top_node->dump(classes, features, "", d, file_arpa);
    } else {
      top_node->dump(classes, features, "", d, file_arpa, ffreqs); // smoothing
    }
  }
  file_arpa << "\\" << "end\\" << std::endl;

  file_arpa.close();

  return 0;
}
