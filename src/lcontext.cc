
// ---------------------------------------------------------------------------
// $Id: lcontext.cc 2426 2009-01-07 12:24:00Z pberck $
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
#include <deque>

#include "qlog.h"
#include "Config.h"
#include "util.h"
#include "lcontext.h"
#include "runrunrun.h"

// ---------------------------------------------------------------------------
//  Code.
// ---------------------------------------------------------------------------

struct lex_elem {
  std::string name;
  double      freq;
  bool operator<(const lex_elem& rhs) const {
    return freq > rhs.freq;
  }
};

/*
  Take words from lexicon if frequency within bounds.
*/
int bounds_from_lex( Logfile& l, Config& c ) {
  l.log( "bounds_from_lex" );
  const std::string& lexicon_filename = c.get_value( "lexicon" );
  int                m                = stoi( c.get_value( "m", "10" ));
  int                n                = stoi( c.get_value( "n", "20" ));
  std::string        range_filename   = lexicon_filename + ".r"+to_str(m)+"n"+to_str(n);

  l.inc_prefix();
  l.log( "lexicon: "+lexicon_filename );
  l.log( "m:       "+to_str(m) );
  l.log( "n:       "+to_str(n) );
  l.log( "OUTPUT:  "+range_filename );
  l.dec_prefix();

  // Load lexicon.
  //
  int wfreq;
  unsigned long total_count = 0;
  std::map<std::string,int> wfreqs; // whole lexicon
  std::vector<lex_elem> lex_vec;
  std::ifstream file_lexicon( lexicon_filename.c_str() );
  if ( ! file_lexicon ) {
    l.log( "ERROR: cannot load lexicon file." );
    return -1;
  }

  std::ofstream range_out( range_filename.c_str(), std::ios::out );
  if ( ! range_out ) {
    l.log( "ERROR: cannot write range file." );
    return -1;
  }

  // Read the lexicon with word frequencies.
  //
  l.log( "Reading lexicon." );
  std::string a_word;
  while ( file_lexicon >> a_word >> wfreq ) {
    wfreqs[a_word] = wfreq;
    lex_elem l;
    l.name = a_word;
    l.freq = wfreq;
    lex_vec.push_back( l );
  }
  file_lexicon.close();
  l.log( "Read lexicon." );

  sort( lex_vec.begin(), lex_vec.end() );
  std::vector<lex_elem>::iterator li;
  li = lex_vec.begin();
  int num = 0;
  while ( li != lex_vec.end() ) {
    int freq = (*li).freq;
    if ( (freq >= m) && (freq < n) ) { // 6 words can be rank 11 ?
      // beter .cnt sorteren, en die frequencies greppen uit het lex.
      // l.log(  (*li).name + "/" + to_str(freq) );
      //
      // Maybe we don't want frequency here. So we can handcraft a
      // list. Or ignore frequencies, but leeave them here...
      //
      range_out << (*li).name << " " << freq << "\n";
      ++num;
    }
    li++;
  }
  range_out.close();
  l.log( "Range contains "+to_str(num)+" items, out of "+to_str(lex_vec.size()) );

  // set RANGE_FILE to range_filename 
  //
  c.add_kv( "range", range_filename );
  l.log( "SET range to "+range_filename );
  return 0;
}
  
/*
  Create data with a the .rng file.

  Start reading the file to be "windowed".
  check each word with .rng vector. Mark if present.
  output vector+word
  next

  end of discourse? reset after nnn words? track changes?
*/
struct gc_elem {
  std::string word;
  int         strength;
  bool operator<(const gc_elem& rhs) const {
    return strength > rhs.strength;
  }
};
bool is_dead(const gc_elem& gce) {
  return gce.strength <= 0;
}

int lcontext( Logfile& l, Config& c ) {
  l.log( "lcontext" );
  const std::string& filename        = c.get_value( "filename" ); // dataset
  const std::string& rng_filename    = c.get_value( "range" );
  int                gcs             = stoi( c.get_value( "gcs",   "3" ));
  int                gcd             = stoi( c.get_value( "gcd", "500" ));
  std::string        output_filename = filename + ".gc" + to_str(gcs)
                                                + "d" + to_str(gcd);

  l.inc_prefix();
  l.log( "filename: "+filename );
  l.log( "range:    "+rng_filename );
  l.log( "gcs:      "+to_str(gcs) );
  l.log( "gcd:      "+to_str(gcd) );
  l.log( "OUTPUT:   "+output_filename );
  l.dec_prefix();

  if ( file_exists( l, c, output_filename ) ) {
    l.log( "OUTPUT exists, not overwriting." );
    c.add_kv( "filename", output_filename );
    l.log( "SET filename to "+output_filename );
    return 0;
  }

  // Open range file.
  //
  std::ifstream file_rng( rng_filename.c_str() );
  if ( ! file_rng ) {
    l.log( "ERROR: cannot load range file." );
    return -1;
  }
  l.log( "Reading range file." );
  std::string a_word;
  long wfreq;
  std::map<std::string, int> range;
  while( file_rng >> a_word >> wfreq ) {
    range[ a_word ] = 1; //ignore frequency?
  }
  l.log( "Loaded range file, "+to_str(range.size())+" items." );
  file_rng.close();

  // What will the output look like....?
  //   Pierre Vinken, 61 years old, will join the board as a 
  //   nonexecutive director Nov. 29. 
  // Imagine, years is focus word. ws2 context, plus global context.
  // Instance: (global) , 61 years
  // global looks like: bit vector? The words or empty words?
  // Imagine the .rng files contains "business, Vinken, good"
  // gc: 0 1 0
  // gc: _ Vinken _
  // Second parameter: size of global context, i.e 10 out of a possible 831.
  //
  // expand global context "vertically", 10x one global context plus rest 
  // of context:
  // _ , 61 years
  // Vinken , 61 years
  // _ , 61 years (double..)
  //
  // Elastigrams!
  //
  // Third parameter: length of global context "trail", when will we 'forget'
  // a word.
  //
  // Make a global_context class which is used in window_lr to update
  // and retrieve.
  // GC->add(word), GC->get_context(4), GC->decay(), etc.
  //
  // Or we give this function another ws3/lnrn dataset, we use the target
  // to generate the g-c, add it to the data, save in new data set.
  //
  // So how is this different from longer context, which doesn't seem
  // to help...?
  //
  std::ofstream file_out( output_filename.c_str(), std::ios::out );
  if ( ! file_out ) {
    l.log( "ERROR: cannot write output file." );
    return -1;
  }
  
  // File to make data from. If this is a wopr-data set, we should
  // only look at the last word (target).
  //
  std::ifstream file_in( filename.c_str() );
  if ( ! file_in ) {
    l.log( "ERROR: cannot load data file." );
    return -1;
  }

  std::vector<std::string> words;
  std::string a_line;
  std::map<std::string, int>::iterator ri;
  // initialize empty global context
  gc_elem empty;
  empty.word = "_";
  empty.strength = 999999;
  std::deque<gc_elem> global_context(10, empty); // hmmm, like this?
  std::deque<gc_elem>::iterator di;
  while( std::getline( file_in, a_line ) ) { 

    Tokenize( a_line, words, ' ' );
    int start = 0;
    if ( true ) {
      start = words.size()-1; // only target
    }
    for( int i = start; i < words.size(); i++ ) {

      //std::find(v1.begin(), v1.end(), string); // slower than find in map?

      std::string wrd = words.at(i);

      ri = range.find( wrd ); // word in data is in .rng list?
      if ( ri != range.end() ) {
	//l.log( "gc word: "+wrd );
	//
	// check if present? or more is better?
	// if present, we can add to strength instead of doubling the entry?
	//
	gc_elem gce;
	gce.word     = wrd;
	gce.strength = gcd;
	global_context.push_front( gce );
	//global_context.insert( global_context.begin(), gce );
      }

      //--
      di = global_context.begin()+gcs;
      int cnt = gcs;
      while ( di != global_context.begin() ) {
	file_out << (*di).word << " "; // << "/" << (*di).strength;
	*di--;
      }
      if ( true ) { // add to data set mode.
	file_out << a_line;
      }
      file_out << std::endl;	
      //--

      //--
      di = global_context.begin()+gcs;
      while ( di != global_context.begin() ) {
	--((*di).strength);
	if ( (*di).strength <= 0 ) {
	  //global_context.erase( di );
	  (*di).strength = 0;
	  //l.log( "Decayed: " + (*di).word );
	}
	*di--;
      }
      //
      std::deque<gc_elem>::iterator end_di;
      end_di = global_context.end();
      di = remove_if( global_context.begin(), end_di, 
		      is_dead );
      global_context.erase( di, end_di );
      //--

    }
    words.clear();
      
  }

  file_in.close();
  file_out.close();

  c.add_kv( "filename", output_filename );
  l.log( "SET filename to "+output_filename );
  return 0;
}

// ---------------------------------------------------------------------------
