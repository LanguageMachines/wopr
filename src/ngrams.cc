// ---------------------------------------------------------------------------
// $Id: ngrams.cc 2426 2009-01-07 12:24:00Z pberck $
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
#include "runrunrun.h"
#include "ngrams.h"

// ---------------------------------------------------------------------------
//  Code.
// ---------------------------------------------------------------------------

// Sentence/line based ngram function.
//
struct ngl_elem {
  long freq;
  int  n;
};
int ngram_list( Logfile& l, Config& c ) {
  l.log( "ngl" );
  const std::string& filename        = c.get_value( "filename" );
  int                n               = stoi( c.get_value( "n", "3" ));
  std::string        output_filename = filename + ".ngl" + to_str(n);
  l.inc_prefix();
  l.log( "filename:  "+filename );
  l.log( "n:         "+to_str(n) );
  l.log( "OUTPUT:    "+output_filename );
  l.dec_prefix();

  std::ifstream file_in( filename.c_str() );
  if ( ! file_in ) {
    l.log( "ERROR: cannot load file." );
    return -1;
  }

  std::string a_line;
  std::vector<std::string> results;
  std::vector<std::string>::iterator ri;
  std::map<std::string,ngl_elem> grams;
  std::map<std::string,ngl_elem>::iterator gi;
  long sum_freq = 0;

  while( std::getline( file_in, a_line )) {

    for ( int i = 1; i <= n; i++ ) {
      results.clear();
      if ( ngram_line( a_line, i, results ) == 0 ) {
	for ( ri = results.begin(); ri != results.end(); ri++ ) {
	  if ( i == 1 ) {
	    ++sum_freq;
	  }
	  std::string cl = *ri;
	  gi = grams.find( cl );
	  if ( gi == grams.end() ) {
	    ngl_elem e;
	    e.freq    = 1;
	    e.n       = i;
	    grams[cl] = e;
	  } else {
	    grams[cl].freq++;
	  }
	}
      }
    }

  }
  file_in.close();

  // a  2
  // a b  2
  // a b c  1
  // a b d  1
  // P(w3 | w1,w2) = C(w1,w2,w3) / C(w1,w2)
  // P(d | a,b) = C(a,b,d) / C(a,b)
  //            = 1 / 2

  // ngram-count -text austen.txt gt1min 0 -gt2min 0 -gt3min 0 
  // -no-sos -no-eos -lm austen.txt.srilm

  std::ofstream file_out( output_filename.c_str(), std::ios::out );
  if ( ! file_out ) {
    l.log( "ERROR: cannot write file." );
    return -1;
  }

  for ( gi = grams.begin(); gi != grams.end(); gi++ ) {
    std::string ngram = (*gi).first;
    ngl_elem e = (*gi).second;
    //file_out << (*gi).first << " " << e.freq << std::endl;
    if ( e.n == 1 ) {
      // srilm takes log10 of probability
      file_out << ngram << " " << e.freq / (float)sum_freq << std::endl;
    } else if ( e.n > 1 ) {
      size_t pos = ngram.rfind( ' ' );
      if ( pos != std::string::npos ) {
	std::string n_minus_1_gram = ngram.substr(0, pos);
	ngl_elem em1 = grams[n_minus_1_gram]; // check if exists
	file_out << ngram << " " << e.freq / (float)em1.freq << std::endl;
      }
    }

  }

  file_out.close();

  c.add_kv( "filename", output_filename );
  l.log( "SET filename to "+output_filename );
  return 0;
}

// ---------------------------------------------------------------------------
