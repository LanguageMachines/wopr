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

#include "math.h"

#include "qlog.h"
#include "Config.h"
#include "util.h"
#include "runrunrun.h"
#include "ngrams.h"

// ---------------------------------------------------------------------------
//  Code.
// ---------------------------------------------------------------------------

// Sentence/line based ngram function. ngl creates a list of n-grams
// from an input text file.
//
struct ngl_elem {
  long freq;
  int  n;
};
int ngram_list( Logfile& l, Config& c ) {
  l.log( "ngl" );
  const std::string& filename        = c.get_value( "filename" );
  int                n               = stoi( c.get_value( "n", "3" ));
  int                fco             = stoi( c.get_value( "fco", "1" ));
  std::string        output_filename = filename + ".ngl" + to_str(n) +
                                                  "f"+to_str(fco);
  l.inc_prefix();
  l.log( "filename:  "+filename );
  l.log( "n:         "+to_str(n) );
  l.log( "fco:       "+to_str(fco) );
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

  l.log( "Reading..." );

  while( std::getline( file_in, a_line )) {

    a_line = trim( a_line, "\n\r " );

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

  l.log( "Writing..." );

  /*
    Smooth here? We have a bunch of local probabilities. Seems silly
    to smooth them with the globaly calculated count*s. A local prob
    of 2 / 40 must be smoothed in a different way than 2 / 3.
    Smoothing per 2-grams, 3-grams, &c? Then we need a different
    data structure, per n-gram.
    Word-trie? Nah.
  */
  
  // Format is:
  // n-gram frequency probability
  //
  for ( gi = grams.begin(); gi != grams.end(); gi++ ) {
    std::string ngram = (*gi).first;
    ngl_elem e = (*gi).second;
    if ( e.n == 1 ) { // unigram
      // srilm saves log10 of probability in its files.
      file_out << ngram << " " << e.freq << " "
	       << e.freq / (float)sum_freq << std::endl;
    } else if ( e.n > 1 ) {
      // filter before we calculate probs?
      if ( e.freq > fco ) { // for n > 1, only if > frequency cut off.
	size_t pos = ngram.rfind( ' ' );
	if ( pos != std::string::npos ) {
	  std::string n_minus_1_gram = ngram.substr(0, pos);
	  ngl_elem em1 = grams[n_minus_1_gram]; // check if exists
	  file_out << ngram << " " << e.freq << " " 
		   << e.freq / (float)em1.freq << std::endl;
	}
      } // freq>1
    } // e.n>1
  }

  file_out.close();

  c.add_kv( "filename", output_filename );
  l.log( "SET filename to "+output_filename );
  return 0;
}

// Return last word in "string with spaces".
// 
void last_word( std::string& str, std::string& res ) {
    size_t pos = str.rfind( ' ' );
    if ( pos != std::string::npos ) {
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
  const std::string& filename        = c.get_value( "filename" );
  const std::string& ngl_filename    = c.get_value( "ngl" );
  const std::string& counts_filename = c.get_value( "counts" );
  int                n               = stoi( c.get_value( "n", "3" ));
  std::string        id             = c.get_value( "id", "" );

  std::string        ngt_filename    = filename;
  std::string        ngp_filename    = filename;
  if ( (id != "") && ( ! contains_id( filename, id)) ) {
    ngt_filename = ngt_filename + "_" + id;
    ngp_filename = ngp_filename + "_" + id;
  }
  ngt_filename = ngt_filename + ".ngt" + to_str(n);
  ngp_filename = ngp_filename + ".ngp" + to_str(n);

  l.inc_prefix();
  l.log( "filename:  "+filename );
  l.log( "ngl file:  "+ngl_filename );
  l.log( "counts:    "+counts_filename );
  l.log( "n:         "+to_str(n) );
  l.log( "id:        "+id );
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

  unsigned long total_count = 0;
  unsigned long N_1 = 0; // Count for p0 estimate.
  std::map<int,double> c_stars;
  int    Nc0;
  double Nc1;
  int    count;
  std::ifstream file_counts( counts_filename.c_str() );
  if ( ! file_counts ) {
    l.log( "NOTICE: cannot read counts file, no smoothing will be applied." ); 
  } else {
    l.log( "Reading counts." );
    while( file_counts >> count >> Nc0 >> Nc1 ) {
      c_stars[count] =  Nc1;
      total_count    += Nc0;
      if ( count == 1 ) {
	N_1 = Nc0;
      }
    }
    file_counts.close();
  }
  double p0 = 1e-6;
  if ( (total_count > 0) && (N_1 > 0) ) {
    p0 = (double)N_1 / (double)total_count;
    // Assume N_0 equals N_1...
    p0 = p0 / (double)N_1;
  }
  l.log( "P(0) = " + to_str(p0) );

  std::ifstream file_ngl( ngl_filename.c_str() );
  if ( ! file_ngl ) {
    l.log( "ERROR: cannot load file." );
    return -1;
  }

  std::string a_line;
  std::vector<std::string> results;
  std::map<std::string,double> ngrams; // NB no ngl_elem here!
  std::map<std::string,double>::iterator gi;

  std::string ngram;
  std::string prob_str;
  std::string freq_str;
  long   freq;
  double prob;
  size_t pos, pos1;

  l.log( "Reading ngrams..." );

  // format: ngram freq prob
  // ngram can contain spaces. freq is ignored at the mo.
  // NB: input and stod are not checked for errors (TODO).
  //
  while( std::getline( file_ngl, a_line ) ) {  
    pos      = a_line.rfind(' ');
    prob_str = a_line.substr(pos+1);
    prob     = stod( prob_str );

    pos1     = a_line.rfind(' ', pos-1);
    //freq_str = a_line.substr(pos1+1, pos-pos1-1);
    ngram    = a_line.substr(0, pos1);
    
    ngrams[ngram] = prob;
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

  // Just need a last-word-of-ngram to size/prob
  // First extract all possible ngrams, then go over input
  // sentence, and for each word, look at n-grams which end
  // with said word.

  std::vector<std::string>::iterator ri;
  std::map<std::string,std::string>::iterator mi;
  std::vector<ngram_elem> best_ngrams;
  std::vector<ngram_elem>::iterator ni;
 
  l.log( "Writing output..." );

  while( std::getline( file_in, a_line )) {

    a_line = trim( a_line, "\n\r " );

    if ( a_line == "" ) {
      continue;
    }

    // Tokenize the input, create a vector for each word
    // with a pointer to best n-gram.
    //
    best_ngrams.clear();

    for ( int i = 1; i <= n; i++ ) { // loop over ngram sizes
      results.clear();
      if ( ngram_line( a_line, i, results ) == 0 ) { // input to ngrams
	int word_idx = i-1;
	for ( ri = results.begin(); ri != results.end(); ri++ ) {
	  std::string cl = *ri;
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
	      // If equal probs, take smallest or largest ngram?
	      // This takes the smallest:
	      //
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
	p = p0;
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
    ngp_out << H << " " 
	    << pplx << " "
	    << a_line << std::endl;
    // NB: pplx is in the end the same as SRILM, we takes log2 and pow(2)
    // in our code, SRILM takes log10s and then pow(10) in the end.
  } // getline
  ngp_out.close();
  file_out.close();
  file_in.close();

  c.add_kv( "ngt_file", ngt_filename );
  l.log( "SET ngt_file to "+ngt_filename );
  c.add_kv( "ngp_file", ngp_filename );
  l.log( "SET ngp_file to "+ngp_filename );
  return 0;
}

/*
  a b c, where c is unknow
  calculate prob. of unknown word after a b, not just "any unknown word".
*/

// ---------------------------------------------------------------------------
