// ---------------------------------------------------------------------------
// $Id: focus.cc 2426 2009-01-07 12:24:00Z pberck $
// ---------------------------------------------------------------------------

/*****************************************************************************
 * Copyright 2007 - 2009 Peter Berck                                         *
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

// ---------------------------------------------------------------------------
//  Includes.
// ---------------------------------------------------------------------------

#if HAVE_CONFIG_H
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

#include "qlog.h"
#include "Config.h"
#include "util.h"
#include "tr.h"
#include "runrunrun.h"

// ---------------------------------------------------------------------------
//  Code.
// ---------------------------------------------------------------------------

int tr( Logfile& l, Config& c ) {
  l.log( "tr" );
  const std::string& filename        = c.get_value( "filename" ); // dataset
  std::string        id              = c.get_value( "id", "" );
  std::string        trs             = c.get_value( "trs", "num" );
  std::string        output_filename = filename + ".tr";

  if ( (id != "") && (contains_id(output_filename, id) == false) ) {
    output_filename = output_filename + "_" + id;
  }

  l.inc_prefix();
  l.log( "filename:   "+filename );
  l.log( "trs:        "+trs ); // target offset
  l.log( "id:         "+id );
  l.log( "OUTPUT:     "+output_filename );
  l.dec_prefix();

  if ( file_exists( l, c, output_filename ) ) {
    l.log( "OUTPUT exists, not overwriting." );
    c.add_kv( "filename", output_filename );
    l.log( "SET filename to "+output_filename );
    return 0;
  }

  l.log( "Processing..." );
  std::ofstream file_out( output_filename.c_str(), std::ios::out );
  if ( ! file_out ) {
    l.log( "ERROR: cannot write output file." );
    return -1;
  }
  
  // We should only look at the last word (target).
  //
  std::ifstream file_in( filename.c_str() );
  if ( ! file_in ) {
    l.log( "ERROR: cannot load data file." );
    return -1;
  }

  std::vector<std::string> words;
  std::string a_line;
  std::string target;
  std::string a_str;
  int         pos;
  std::map<std::string, int>::iterator ri;

  while( std::getline( file_in, a_line ) ) { 

    words.clear();
    Tokenize( a_line, words, ' ' );
    int t_pos = words.size()-1; // Target position
    for ( int i=0; i < t_pos; i++ ) {
      
      char c = words.at(i)[0];
      if ( (c >= '0') && (c <= '9') ) {
	file_out << "<num> ";
      } else {
	file_out << words.at(i) << " ";
      }
      
    }
    file_out << words.at(t_pos) << std::endl;
  }

  file_in.close();
  file_out.close();

  c.add_kv( "filename", output_filename );
  l.log( "SET filename to "+output_filename );
  return 0;
}

// ---------------------------------------------------------------------------
