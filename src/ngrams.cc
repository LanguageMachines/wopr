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

void last_word( std::string& str, std::string& res ) {
    size_t pos = str.rfind( ' ' );
    if ( pos != std::string::npos ) {
      //res = str.substr(0, pos);
      res = str.substr(pos+1);
    } else {
      res = str;
    }
}

struct ngram_elem {
  double p;
  int    n;
  std::string ngram;
};
int ngram_test( Logfile& l, Config& c ) {
  l.log( "ngt" );
  const std::string& filename     = c.get_value( "filename" );
  const std::string& ngl_filename = c.get_value( "ngl" );
  int                n            = stoi( c.get_value( "n", "3" ));
  std::string        ngt_filename = filename + ".ngt" + to_str(n);
  std::string        ngp_filename = filename + ".ngp" + to_str(n);

  l.inc_prefix();
  l.log( "filename:  "+filename );
  l.log( "ngl file:  "+ngl_filename );
  l.log( "n:         "+to_str(n) );
  l.log( "OUTPUT:    "+ngt_filename );
  l.log( "OUTPUT:    "+ngp_filename );
  l.dec_prefix();

  if ( file_exists(l,c,ngt_filename) ) {
    l.log( "OUTPUT files exist, not overwriting." );
    c.add_kv( "ngt_file", ngt_filename );
    l.log( "SET ngt_file to "+ngt_filename );
    c.add_kv( "ngp_file", ngp_filename );
    l.log( "SET ngp_file to "+ngp_filename );
    return 0;
  }

  std::ifstream file_ngl( ngl_filename.c_str() );
  if ( ! file_ngl ) {
    l.log( "ERROR: cannot load file." );
    return -1;
  }

  std::string a_line;
  std::vector<std::string> results;
  std::map<std::string,double> ngrams; // NB no ngl_elem here!
  std::map<std::string,double>::iterator gi;

  while( std::getline( file_ngl, a_line )) {

    size_t pos = a_line.rfind( ' ' );
    if ( pos != std::string::npos ) {
      std::string ngram    = a_line.substr(0, pos);
      std::string prob_str = a_line.substr(pos+1);
      double prob = stod(prob_str);
      ngrams[ngram] = prob;
    }
  }
  file_ngl.close();

  std::ifstream file_in( filename.c_str() );
  if ( ! file_in ) {
    l.log( "ERROR: cannot load file." );
    return -1;
  }

  std::ofstream file_out( ngt_filename.c_str(), std::ios::out );
  if ( ! file_out ) {
    l.log( "ERROR: cannot write .ngt output file." );
    return -1;
  }
  file_out << "# word prob n ngram" << std::endl;

  std::ofstream ngp_out( ngp_filename.c_str(), std::ios::out );
  if ( ! ngp_out ) {
    l.log( "ERROR: cannot write .ngp output file." );
    return -1;
  }

  // Maybe I need a reverse index to get some speed...
  // We need to find n-grams which END with a certain word.
  // map<std::string,std::vector<std::string>> oid.

  // Just need a last-word-of-ngram to size/prob

  // First extract all possible ngrams, then go over input
  // sentence, and for each word, look at n-grams which end
  // with said word.

  std::vector<std::string>::iterator ri;

  std::map<std::string,std::string>::iterator mi;
  std::vector<ngram_elem> best_ngrams;
  std::vector<ngram_elem>::iterator ni;
 
  while( std::getline( file_in, a_line )) {

    if ( a_line == "" ) {
      continue;
    }

    // Tokenize the input, create a vector for each word
    // with a pointer to best n-gram.
    //
    best_ngrams.clear();

    for ( int i = 1; i <= n; i++ ) {
      results.clear();
      if ( ngram_line( a_line, i, results ) == 0 ) {
	int word_idx = i-1;
	for ( ri = results.begin(); ri != results.end(); ri++ ) {
	  std::string cl = *ri;
	  //l.log( cl );
	  //l.log( to_str(word_idx) );
	  gi = ngrams.find( cl );
	  if ( gi != ngrams.end() ) {
	    //l.log( "Checking: " + (*gi).first + "/" + to_str((*gi).second) );
	    std::string matchgram = (*gi).first;
	    std::string lw;
	    last_word( matchgram, lw );
	    //
	    // Store in the best_ngrams vector.
	    //
	    if ( i == 1 ) { // start with unigrams, no element present.
	      ngram_elem ne;
	      ne.p = (*gi).second;
	      ne.n = i;
	      ne.ngram = matchgram;
	      //l.log( to_str(ne.p));
	      best_ngrams.push_back(ne);
	    } else {
	      ngram_elem& ne = best_ngrams.at(word_idx);
	      if ( (*gi).second > ne.p ) { // Higher prob than stored.
		ne.p = (*gi).second;
		ne.n = i;
		ne.ngram = matchgram;
		//l.log( "Replace with: "+to_str(ne.p));
	      }
	    }
	    
	  } else {
	    //l.log( "unknown" );
	    if ( i == 1 ) { // start with unigrams
	      ngram_elem ne;
	      ne.p = 0;
	      ne.n = 0;
	      best_ngrams.push_back(ne);
	    }

	  }
	  ++word_idx;
	} // for
      }
    }
    //
    // Print.
    //
    results.clear();
    double H  = 0.0;
    int    wc = 0;
    Tokenize( a_line, results, ' ' );
    for( ni = best_ngrams.begin(); ni != best_ngrams.end(); ++ni ) {
      double p = (*ni).p;
      if ( p == 0 ) {
	p = 1e-5; // TODO: smoothing, p(0) calculations, ...
      }
      file_out << results.at(wc) << " "
	       << p << " "
	       << (*ni).n << " "
	       << (*ni).ngram
	       << std::endl;
      /*l.log( results.at(wc) + ":" + to_str(p) + "/" + to_str((*ni).n)
	+ "   " + (*ni).ngram );*/
      H += log2(p);
      ++wc;
    }
    double pplx = pow( 2, -H/(double)wc );
    //l.log( "H="+to_str(H) );
    //l.log( "pplx="+to_str(pplx) );
    ngp_out << H << " " 
	    << pplx << " "
	    << a_line << std::endl;
    // NB: pplx is in the end the same as SRILM, we takes log2 and pow(2)
    // in our code, SRILM takes log10s and then pow(10) in the end.
  }
  ngp_out.close();
  file_out.close();
  file_in.close();

  c.add_kv( "ngt_file", ngt_filename );
  l.log( "SET ngt_file to "+ngt_filename );
  c.add_kv( "ngp_file", ngp_filename );
  l.log( "SET ngp_file to "+ngp_filename );
  return 0;
}


// ---------------------------------------------------------------------------
