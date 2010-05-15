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

#ifdef TIMBL
# include "timbl/TimblAPI.h"
#endif

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

// New focus function.
//
int focus( Logfile& l, Config& c ) {
  l.log( "focus" );

  const std::string& filename        = c.get_value( "filename" ); // dataset
  const std::string& focus_filename  = c.get_value( "focus" );
  int                fco             = stoi( c.get_value( "fco", "0" ));
  int                fmode           = stoi( c.get_value( "fcm", "0" ));
  std::string        id              = c.get_value( "id", "" );
  std::string        dflt            = c.get_value( "default", "dflt" );
  const std::string& timbl           = c.get_value( "timbl" ); 

  std::string        output_basename = filename;
  std::string        output_filename = output_basename + ".fcs"+to_str(fco);
  std::string        kvs_filename    = output_basename + ".kvs";
  std::string        combined_class  = c.get_value( "PID", "0000" );

  // Note for fmode=seperate, this doesn't work.
  // Choose depending on modus...
  //
  if ( fmode == 0 ) {
    if ( (id != "") && (contains_id(output_filename, id) == false) ) {
      output_filename = output_filename + "_" + id;
    }
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
  l.log( "default:    "+dflt );
  l.log( "timbl:      "+timbl );
  l.log( "OUTPUT:     "+output_filename );
  l.dec_prefix();

  // NB when fmode = 1, seperate list with words, one for each dataset
  //
  if ( fmode == 0 ) {
    if ( file_exists( l, c, output_filename ) ) {
      l.log( "OUTPUT exists, not overwriting." );
      c.add_kv( "filename", output_filename );
      l.log( "SET filename to "+output_filename );
      return 0;
    }
  }

  // List to focus on.
  //
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

  // Contains all the filenames of the datafiles we will create.
  //
  std::map<std::string, std::string> output_files;

  // Create a default/left-over file.
  // What if this is a word? Should be configurable?
  //
  std::string output_filename_dflt = output_filename + "_" + dflt;
  output_files[ dflt ] = output_filename_dflt;

  if ( fmode == 0 ) { // one only
    output_files[ combined_class ] = output_filename;
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

    ri = focus_words.find( target ); // Is it in the focus list?
    if ( ri != focus_words.end() ) {
      //l.log( "FOUND: "+target+" in "+a_line );
      is_dflt = 1;
      if ( fmode == 0 ) { 
	std::ofstream file_out( output_files[combined_class].c_str(), std::ios::app );
	if ( ! file_out ) {
	  l.log( "ERROR: cannot write output file ["+output_files[combined_class]+"]." );
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
      std::ofstream file_out( output_files[dflt].c_str(), std::ios::app );
      if ( ! file_out ) {
	l.log( "ERROR: cannot write output file." );
	return -1;
      }
      file_out << a_line << std::endl;
      file_out.close(); // must be inefficient...
    }
    
  }

  file_in.close();

  // Now train them all, or create a script to train...? or train,
  // and create kvs script (best).
  //
#ifdef TIMBL

  std::ofstream kvs_out( kvs_filename.c_str(), std::ios::out );
  if ( ! kvs_out ) {
    l.log( "ERROR: cannot write kvs output file." );
    return -1;
  }

  Timbl::TimblAPI       *My_Experiment;
  std::map<std::string,std::string>::iterator ofi;
  for( ofi = output_files.begin(); ofi != output_files.end(); ofi++ ) {
    std::string focus_word      = (*ofi).first;
    std::string output_filename = (*ofi).second;

    std::string t_ext = timbl;
    std::string ibase_filename = output_filename;
    t_ext.erase( std::remove(t_ext.begin(), t_ext.end(), ' '), t_ext.end());
    if ( t_ext != "" ) {
      ibase_filename = ibase_filename+"_"+t_ext;
    }
    ibase_filename = ibase_filename+".ibase";
    
    l.log( output_filename+"/"+ibase_filename );

    My_Experiment = new Timbl::TimblAPI( timbl );
    (void)My_Experiment->Learn( output_filename );
    My_Experiment->WriteInstanceBase( ibase_filename );
    delete My_Experiment;

    if ( fmode == 1 ) {
      kvs_out << "classifier:" << focus_word << std::endl;
      kvs_out << "ibasefile:" << ibase_filename << std::endl;
      kvs_out << "timbl:" << timbl << std::endl;
      if ( focus_word == "dflt" ) {
	kvs_out << "gatedefault:1" << std::endl;
      } else {
	kvs_out << "gatetrigger:" << focus_word << std::endl;
	kvs_out << "gatepos:" << fco << std::endl;
      }
      kvs_out << std::endl;
    }
  }
  
  kvs_out.close();
#endif
  

  c.add_kv( "filename", output_filename );
    l.log( "SET filename to "+output_filename );
  return 0;
}

// ---------------------------------------------------------------------------
