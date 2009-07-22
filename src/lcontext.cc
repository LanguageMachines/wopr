
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

#include "qlog.h"
#include "Config.h"
#include "util.h"
#include "lcontext.h"

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

int bounds_from_lex( Logfile& l, Config& c ) {
  l.log( "bounds_from_lex" );
  const std::string& lexicon_filename = c.get_value( "lexicon" );
  int                m                = stoi( c.get_value( "m", "10" ));
  int                n                = stoi( c.get_value( "n", "20" ));

  l.inc_prefix();
  l.log( "lexicon: "+lexicon_filename );
  l.log( "m      : "+to_str(m) );
  l.log( "n      : "+to_str(n) );
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
  l.log( "Read lexicon (total_count="+to_str(total_count)+")." );

  sort( lex_vec.begin(), lex_vec.end() );
  std::vector<lex_elem>::iterator li;
  li = lex_vec.begin();
  int rank = 0;
  while ( li != lex_vec.end() ) {
    if ( (rank > m) && (rank <= n) ) { // 6 words can be rank 11 ?
      l.log( to_str(rank) + ":" + (*li).name + "/" + to_str((*li).freq) );
    }
    ++rank;
    li++;
  }
  return 0;
}
  


// ---------------------------------------------------------------------------
