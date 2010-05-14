// ---------------------------------------------------------------------------
// $Id$
// ---------------------------------------------------------------------------

/*****************************************************************************
 * Copyright 2007 - 2010 Peter Berck                                         *
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

int focus_old( Logfile& l, Config& c ) {
  l.log( "focus" );
  const std::string& filename        = c.get_value( "filename" ); // dataset
  const std::string& focus_filename  = c.get_value( "focus" );
  int                fco             = stoi( c.get_value( "fco", "0" ));
  std::string        id              = c.get_value( "id", "" );
  std::string        output_filename = filename + ".fcs"+to_str(fco);

  if ( (id != "") && (contains_id(output_filename, id) == false) ) {
    output_filename = output_filename + "_" + id;
  }

  if ( fco < 0 ) {
    l.log( "ERROR: fco cannot be less than zero." );
    return 1;
  }

  l.inc_prefix();
  l.log( "filename:   "+filename );
  l.log( "focus:      "+focus_filename );
  l.log( "fco:        "+to_str(fco) ); // target offset
  l.log( "id:         "+id );
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
  std::string a_line;
  std::vector<std::string> words;
  std::map<std::string, int> focus_words;
  while( std::getline( file_fcs, a_line )) {
    words.clear();
    Tokenize( a_line, words, ' ' );
    std::string a_word = words[0]; //assume first is the word, rest may be freqs
    
    focus_words[ a_word ] = 1; // PJB maybe we want freq. here to filter on
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

  std::string target;
  std::string a_str;
  int         pos;
  std::map<std::string, int>::iterator ri;

  while( std::getline( file_in, a_line ) ) { 

    words.clear();
    Tokenize( a_line, words, ' ' );
   
    pos = words.size()-1-fco;
    pos = (pos < 0) ? 0 : pos;
    target = words[pos];
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

// If we want a "left over" Ibase we do need to do it like this.

// In script, when learning, move small datasets to default data
// set and skip.

int focus( Logfile& l, Config& c ) {
  l.log( "focus" );
  const std::string& filename        = c.get_value( "filename" ); // dataset
  const std::string& focus_filename  = c.get_value( "focus" );
  int                fco             = stoi( c.get_value( "fco", "0" ));
  int                fmode           = stoi( c.get_value( "fcm", "0" )); //focus mode
  std::string        id              = c.get_value( "id", "" );
  std::string        output_filename = filename + ".fcs"+to_str(fco);

  // Note for fmode=seperate, this doesn't work.
  // Choose depending on modus...
  //
  if ( (id != "") && (contains_id(output_filename, id) == false) ) {
    output_filename = output_filename + "_" + id;
  }

  if ( fco < 0 ) {
    l.log( "ERROR: fco cannot be less than zero." );
    return 1;
  }

  l.inc_prefix();
  l.log( "filename:   "+filename );
  l.log( "focus:      "+focus_filename );
  l.log( "fco:        "+to_str(fco) ); // target offset
  l.log( "fcm:        "+to_str(fmode) );
  l.log( "id:         "+id );
  l.log( "OUTPUT:     "+output_filename );
  l.dec_prefix();

  // NB when fmode = 1, seperate list with words, one for each dataset
  //
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

  std::string a_line;
  std::vector<std::string> words;
  std::map<std::string, int> focus_words;

  while( std::getline( file_fcs, a_line )) {
    words.clear();
    Tokenize( a_line, words, ' ' );
    std::string a_word = words[0]; //assume first is the word, rest may be freqs
    
    focus_words[ a_word ] = 1; // PJB maybe we want freq. here to filter on
  }
  l.log( "Loaded focus file, "+to_str(focus_words.size())+" items." );
  file_fcs.close();

  l.log( "Processing..." );

  // Can I open n output files here? What is n > many?
  //
  std::map<std::string, std::string> output_files;

  // Create a default/left-over file.
  // What if this is a word?
  //
  std::string output_filename_dflt = output_filename + "_" + "dflt";
  output_files[ "dflt" ] = output_filename_dflt;

  if ( fmode == 0 ) { // one only
    output_files[ "a" ] = output_filename;
  } else if ( fmode == 1 ) {
    std::map<std::string,int>::iterator ofi;
    for( ofi = focus_words.begin(); ofi != focus_words.end(); ofi++ ) {
      std::string fw = (*ofi).first;
      std::string output_filename_fw = output_filename + "_" + fw;
      l.log( "OPEN: " + output_filename_fw );
      output_files[ fw ] = output_filename_fw;
    }
  }
  
  // Data file, already windowed.
  //
  std::ifstream file_in( filename.c_str() );
  if ( ! file_in ) {
    l.log( "ERROR: cannot load data file." );
    return -1;
  }

  std::string target;
  std::string a_str;
  int         pos;
  std::map<std::string, int>::iterator ri;
  int is_dflt;

  while( std::getline( file_in, a_line ) ) { 

    words.clear();
    Tokenize( a_line, words, ' ' );
   
    pos = words.size()-1-fco;
    pos = (pos < 0) ? 0 : pos;
    target = words[pos]; // target is the focus "target"
    a_str  = words[0];
    is_dflt = 0;

    ri = focus_words.find( target );
    if ( ri != focus_words.end() ) {
      //l.log( "FOUND: "+target+" in "+a_line );
      is_dflt = 1;
      if ( fmode == 0 ) {
	std::ofstream file_out( output_files["a"].c_str(), std::ios::app );
	if ( ! file_out ) {
	  l.log( "ERROR: cannot write output file." );
	  return -1;
	}
	file_out << a_line << std::endl;
	file_out.close(); // must be inefficient...
      } else if ( fmode == 1 ) {
	std::ofstream file_out( output_files[target].c_str(), std::ios::app );
	if ( ! file_out ) {
	  l.log( "ERROR: cannot write output file." );
	  return -1;
	}
	file_out << a_line << std::endl;
	file_out.close(); // must be inefficient...
      }
    }

    if ( is_dflt == 0 ) {
      std::ofstream file_out( output_files["dflt"].c_str(), std::ios::app );
      if ( ! file_out ) {
	l.log( "ERROR: cannot write output file." );
	return -1;
      }
      file_out << a_line << std::endl;
      file_out.close(); // must be inefficient...
    }
    
  }

  file_in.close();

  c.add_kv( "filename", output_filename );
    l.log( "SET filename to "+output_filename );
  return 0;
}

// ---------------------------------------------------------------------------
