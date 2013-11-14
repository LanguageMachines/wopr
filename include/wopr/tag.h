// $Id$
//

/*****************************************************************************
 * Copyright 2008 - 2011 Peter Berck                                                *
 *                                                                           *
 * This file is part of wopr.                                                *
 *                                                                           *
 * wpred is free software; you can redistribute it and/or modify it     *
 * under the terms of the GNU General Public License as published by the     *
 * Free Software Foundation; either version 2 of the License, or (at your    *
 * option) any later version.                                                *
 *                                                                           *
 * wpred is distributed in the hope that it will be useful, but WITHOUT *
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or     *
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License      *
 * for more details.                                                         *
 *                                                                           *
 * You should have received a copy of the GNU General Public License         *
 * along with wpred; if not, write to the Free Software Foundation,     *
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA               *
 *****************************************************************************/

#ifndef _TAG_H
#define _TAG_H


#include "qlog.h"
#include "Config.h"

class Node;

class Arc {

 public:
  Node *node; // to this node.
  int label;

  Arc( int, Node* );

  int get_label();
  Node* get_node();
};

class Node {

 public:
  int classhash;
  std::vector<int> dist; // hash with classhash, freq, ...
  std::vector<Arc*> arcs;

  Node( int );

  void set_defaultclass( int );
  void add_dist( int, int );
  void add_arc( int, Node* );
  std::string info();
  std::string get_dist();
  void smooth_dist( std::map<int, double>&, std::vector<double>& );
  std::string get_dist( std::map<int, double>& );
  void dump();
  void dump(std::map<int, std::string>&, std::map<int, std::string>&, std::string, int, std::ofstream& );
  void dump(std::map<int, std::string>&, std::map<int, std::string>&, std::string, int, std::ofstream&, std::map<int, double>& );
};

int tag( Logfile&, Config& );

#endif
