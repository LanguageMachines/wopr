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
#include "focus.h"
#include "runrunrun.h"

// ---------------------------------------------------------------------------
//  Code.
// ---------------------------------------------------------------------------

int focus( Logfile& l, Config& c ) {
  l.log( "focus" );
  const std::string& filename        = c.get_value( "filename" ); // dataset
  const std::string& focus_filename  = c.get_value( "focus" );
  std::string        output_filename = filename + ".fcs";

  l.inc_prefix();
  l.log( "filename:   "+filename );
  l.log( "focus:      "+focus_filename );
  l.log( "OUTPUT:     "+output_filename );
  l.dec_prefix();


  if ( file_exists( l, c, output_filename ) ) {
    l.log( "OUTPUT exists, not overwriting." );
    c.add_kv( "filename", output_filename );
    l.log( "SET filename to "+output_filename );
    return 0;
  }

  std::ifstream file_fcs( focus_filename.c_str() );
  if ( ! file_fcs ) {
    l.log( "ERROR: cannot load focus file." );
    return -1;
  }
  l.log( "Reading focus file." );
  std::string a_word;
  std::map<std::string, int> focus_words;
  while( file_fcs >> a_word ) {
    focus_words[ a_word ] = 1; // PJB maybe we want frequency/.. here to filter on
  }
  l.log( "Loaded focus file, "+to_str(focus_words.size())+" items." );
  file_fcs.close();

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
  std::map<std::string, int>::iterator ri;

  while( std::getline( file_in, a_line ) ) { 

    words.clear();
    Tokenize( a_line, words, ' ' );
   
    target = words[words.size()-1];
    a_str  = words[0];

    ri = focus_words.find( target );
    if ( ri != focus_words.end() ) {
      file_out << a_line << std::endl;
    }
  }

  file_in.close();
  file_out.close();

  c.add_kv( "filename", output_filename );
    l.log( "SET filename to "+output_filename );
  return 0;
}

// ---------------------------------------------------------------------------
