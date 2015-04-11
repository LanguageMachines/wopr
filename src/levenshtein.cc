// ---------------------------------------------------------------------------
// $Id$
// ---------------------------------------------------------------------------

/*****************************************************************************
 * Copyright 2007 - 2015 Peter Berck                                         *
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

#ifdef HAVE_CONFIG_H
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

#if defined(HAVE_TR1_UNORDERED_SET)
#include <tr1/unordered_set>
#else
#include <unordered_set>
#endif

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <functional>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#ifdef HAVE_ICU
#define U_CHARSET_IS_UTF8 1
#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "unicode/locid.h"
#include "unicode/ustring.h"
#include "unicode/ucnv.h"
#include "unicode/unistr.h"
#include "unicode/ucol.h"
#endif

#include "wopr/qlog.h"
#include "wopr/util.h"
#include "wopr/Config.h"
#include "wopr/runrunrun.h"
#include "wopr/tag.h"
#include "wopr/Classifier.h"
#include "wopr/Multi.h"
#include "wopr/server.h"
#include "wopr/levenshtein.h"
#include "wopr/elements.h"
#include "wopr/cache.h"
#include "wopr/Expert.h"

#ifdef TIMBL
# include "timbl/TimblAPI.h"
#endif

#ifdef TIMBLSERVER
#include "wopr/SocketBasics.h"
#endif

#define BACKLOG 5        // how many pending connections queue will hold
#define MAXDATASIZE 2048 // max number of bytes we can get at once

char toLowerCase(char c) {
    return char( std::tolower(static_cast<unsigned char>( c )));
}
std::string toLowerCase( std::string const& s ) {
  std::string result( s.length(), '\0' );
  setlocale( LC_ALL, "" );
  std::transform( s.begin(), s.end(), result.begin(), std::ptr_fun<char>(toLowerCase) );
  return result;
}

int min3( int a, int b, int c ) {
  int mi;
  
  mi = a;
  if (b < mi) {
    mi = b;
  }
  if (c < mi) {
    mi = c;
  }
  return mi;
}

#ifndef HAVE_ICU
int lev_distance(const std::string& source, const std::string& target, int mld, bool cs) {

  // Step 1

  const int n = source.length();
  const int m = target.length();
  if (n == 0) {
    return m;
  }
  if (m == 0) {
    return n;
  }

  // Short cut, we ignore LDs > mld, so if the words differ so many characters
  // we already know enough. The value returned is an approximation...
  if ( abs(n-m) > mld ) {
    return abs(n-m);
  }

  typedef std::vector< std::vector<int> > Tmatrix;
  Tmatrix matrix(n+1);

  // Size the vectors in the 2.nd dimension. Unfortunately C++ doesn't
  // allow for allocation on declaration of 2.nd dimension of vec of vec

  for (int i = 0; i <= n; i++) {
    matrix[i].resize(m+1);
  }

  // Step 2

  for (int i = 0; i <= n; i++) {
    matrix[i][0]=i;
  }

  for (int j = 0; j <= m; j++) {
    matrix[0][j]=j;
  }

  // Step 3

  for (int i = 1; i <= n; i++) {

    const char s_i = source[i-1];

    // Step 4

    for (int j = 1; j <= m; j++) {

      const char t_j = target[j-1];

      // Step 5

      int cost;
      if ( cs ) { //case sensitive
	if (s_i == t_j) {
	  cost = 0;
	} else {
	  cost = 1;
	}
      } else {
	if ( toLowerCase(s_i) == toLowerCase(t_j) ) {
	  cost = 0;
	} else {
	  cost = 1;
	}
      }

      // Step 6

      int above = matrix[i-1][j];
      int left  = matrix[i][j-1];
      int diag  = matrix[i-1][j-1];

      //const int cell  = min( above + 1, min(left + 1, diag + cost));
      int cell  = min3( above + 1, left + 1, diag + cost );

      // Step 6A: Cover transposition, in addition to deletion,
      // insertion and substitution. This step is taken from:
      // Berghel, Hal ; Roach, David : "An Extension of Ukkonen's
      // Enhanced Dynamic Programming ASM Algorithm"
      // (http://www.acm.org/~hlb/publications/asm/asm.html)

      if ( i>2 && j>2 ) {
        int trans = matrix[i-2][j-2] + 1;

	// Code gives LD:2 to a transposition. If TRANSPLD2
	// is not defined, a transposition is LD:1
#ifdef TRANSPLD2
	if ( cs ) { // case sensitive
	  if ( source[i-2] != t_j ) {
	    trans++;
	  }
	  if ( s_i != target[j-2] ) {
	    trans++;
	  }
	} else {
	  if ( toLowerCase(source[i-2]) != toLowerCase(t_j) ) {
	    trans++;
	  }
	  if ( toLowerCase(s_i) != toLowerCase(target[j-2]) ) {
	    trans++;
	  }
	}
#endif
	if ( cs ) { // case sensitive
	  if ( source[i-2] != t_j ) {
	    if ( s_i != target[j-2] ) {
	      trans++;
	    }
	  }
	} else {
	  if ( toLowerCase(source[i-2]) != toLowerCase(t_j) ) {
	    if ( toLowerCase(s_i) != toLowerCase(target[j-2]) ) {
	      trans++;
	    }
	  }
	}

        if ( cell > trans ) {
	  cell = trans;
	}
      }

      matrix[i][j] = cell;
    }
  }

  // Step 7

  return matrix[n][m];
}
#else
int lev_distance(const std::string& source, const std::string& target, int mld, bool cs) {

  UnicodeString u_source = UnicodeString::fromUTF8(source);
  UnicodeString u_target = UnicodeString::fromUTF8(target);

  // Step 1

  const int n = u_source.length();
  const int m = u_target.length();

  if (n == 0) {
    return m;
  }
  if (m == 0) {
    return n;
  }

  // Short cut, we ignore LDs > mld, so if the words differ so many characters
  // we already know enough. The value returned is an approximation...
  if ( abs(n-m) > mld ) {
    return abs(n-m);
  }

  typedef std::vector< std::vector<int> > Tmatrix;
  Tmatrix matrix(n+1);

  // Size the vectors in the 2.nd dimension. Unfortunately C++ doesn't
  // allow for allocation on declaration of 2.nd dimension of vec of vec

  for (int i = 0; i <= n; i++) {
    matrix[i].resize(m+1);
  }

  // Step 2

  for (int i = 0; i <= n; i++) {
    matrix[i][0]=i;
  }

  for (int j = 0; j <= m; j++) {
    matrix[0][j]=j;
  }

  // Step 3

  for (int i = 1; i <= n; i++) {

    const UChar s_i = u_source.charAt(i-1);

    // Step 4

    for (int j = 1; j <= m; j++) {

      const UChar t_j = u_target.charAt(j-1);

      // Step 5

      int cost;
      if ( cs ) { // case sensitive
	if (s_i == t_j) {
	  cost = 0;
	} else {
	  cost = 1;
	}
      } else {
	//UChar32 	u_foldCase (UChar32 c, uint32_t options)
	if (u_foldCase(s_i, 0) == u_foldCase(t_j, 0)) {
	  cost = 0;
	} else {
	  cost = 1;
	}
      }

      // Step 6

      int above = matrix[i-1][j];
      int left  = matrix[i][j-1];
      int diag  = matrix[i-1][j-1];

      //const int cell  = min( above + 1, min(left + 1, diag + cost));
      int cell  = min3( above + 1, left + 1, diag + cost );

      // Step 6A: Cover transposition, in addition to deletion,
      // insertion and substitution. This step is taken from:
      // Berghel, Hal ; Roach, David : "An Extension of Ukkonen's
      // Enhanced Dynamic Programming ASM Algorithm"
      // (http://www.acm.org/~hlb/publications/asm/asm.html)

#ifndef TRANSPLD2
      if ( i>2 && j>2 ) {
        int trans = matrix[i-2][j-2] + 1;

	// Code gives LD:2 to a transposition. If TRANSPLD2
	// is not defined, a transposition is LD:1

	if ( cs ) { // case sensitive
	  if ( u_source.charAt(i-2) != t_j ) {
	    //std::cerr << char(t_j) << char(u_source.charAt(i-2)) << std::endl;
	    trans++;
	  }
	  if ( s_i != u_target.charAt(j-2) ) {
	    trans++;
	  }

	  if ( u_source.charAt(i-2) != t_j ) {
	    if ( s_i != u_target.charAt(j-2) ) {
	      trans++;
	    }
	  }
	} else { // not cs
	  if ( u_foldCase(u_source.charAt(i-2), 0) != u_foldCase(t_j, 0) ) {
	    //std::cerr << char(t_j) << char(u_source.charAt(i-2)) << std::endl;
	    trans++;
	  }
	  if ( u_foldCase(s_i, 0) != u_foldCase(u_target.charAt(j-2), 0) ) {
	    trans++;
	  }

	  if ( u_foldCase(u_source.charAt(i-2), 0) != u_foldCase(t_j, 0) ) {
	    if ( u_foldCase(s_i, 0) != u_foldCase(u_target.charAt(j-2), 0) ) {
	      trans++;
	    }
	  }
	}

        if ( cell > trans ) {
	  cell = trans;
	}
      }
#endif
      matrix[i][j] = cell;
    }
  }

  // Step 7

  return matrix[n][m];
}
#endif

// call with "wopr -r wopr -p c:ld"
int levenshtein( Logfile& l, Config& c ) {

  int mld = 5;
  int cs = true;
  l.log( "gumbo-gambol: "+to_str( lev_distance( "gumbo", "gambol", mld, cs )));
  l.log( "peter-petr: "+to_str( lev_distance( "peter", "petr", mld, cs )));
  l.log( "switch-swicth: "+to_str( lev_distance( "switch", "swicth", mld, cs )));
  l.log( "noswitch-noswitch: "+to_str( lev_distance( "noswitch", "noswitch", mld, cs )));
  l.log( "123-312: "+to_str( lev_distance( "123", "312", mld, cs )));
  l.log( "123-321: "+to_str( lev_distance( "123", "321", mld, cs )));
  l.log( "12-123: "+to_str( lev_distance( "12", "123", mld, cs )));
  l.log( "123-12: "+to_str( lev_distance( "123", "12", mld, cs )));
  l.log( "aaaa-aaaaa: "+to_str( lev_distance( "aaaa", "aaaaa", mld, cs )));
  l.log( "aaaaa-aaaa: "+to_str( lev_distance( "aaaaa", "aaaa", mld, cs )));
  l.log( "aaaabbbb-aaaabbbb: "+to_str( lev_distance( "aaaabbbb", "aaaabbbb", mld, cs )));
  l.log( "aaaabbbb-aaababbb: "+to_str( lev_distance( "aaaabbbb", "aaababbb", mld, cs )));
  l.log( "aaaabbbb-aabababb: "+to_str( lev_distance( "aaaabbbb", "aabababb", mld, cs )));
  l.log( "aaaabbbb-bbbbaaaa: "+to_str( lev_distance( "aaaabbbb", "bbbbaaaa", mld, cs )));
  l.log( "sor-sör: "+to_str( lev_distance( "sor", "sör", mld, cs )));
  l.log( "transpåöx-transpöåx: "+to_str( lev_distance( "transpåöx", "transpöåx", mld, cs )));
  l.log( "källardörrhål-källardörrhål: "+to_str( lev_distance( "källardörrhål", "källardörhål", mld, cs )));
  l.log( "至高-高至"+to_str( lev_distance( "至高", "高至", mld, cs )));
  l.log( "Sor-sor (cs): "+to_str( lev_distance( "Sor", "sor", mld, true )));
  l.log( "Sor-sor (  ): "+to_str( lev_distance( "Sor", "sor", mld, false )));
  l.log( "ABC-abc (cs): "+to_str( lev_distance( "ABC", "abc", mld, true )));
  l.log( "ACB-abc (  ): "+to_str( lev_distance( "ACB", "abc", mld, false )));

  return 0;
}

void distr_examine( const Timbl::ValueDistribution *vd, const std::string target,
		    double& entropy) {
  int cnt = 0;
  int distr_count = 0;
  int target_freq = 0;
  int answer_freq = 0;
  double prob            = 0.0;
  double target_distprob = 0.0;
  //double entropy         = 0.0;
  bool in_distr          = false;

  cnt = vd->size();
  distr_count = vd->totalSize();

  // Check if target word is in the distribution.
  //
  Timbl::ValueDistribution::dist_iterator it = vd->begin();
  while ( it != vd->end() ) {
    //const Timbl::TargetValue *tv = it->second->Value();

    std::string tvs  = it->second->Value()->Name();
    double      wght = it->second->Weight();

    // Prob. of this item in distribution.
    //
    prob     = (double)wght / (double)distr_count;
    entropy -= ( prob * log2(prob) );

    if ( tvs == target ) { // The correct answer was in the distribution!
      target_freq = wght;
      in_distr = true;
    }

    ++it;
  }
  target_distprob = (double)target_freq / (double)distr_count;
}

// The core, do the spelling correction on the distribution.
// tv = My_Experiment->Classify( a_line, vd );
//
void distr_spelcorr( const Timbl::ValueDistribution *vd, const std::string& target, std::map<std::string,int>& wfreqs,
					 std::vector<distr_elem*>& distr_vec, int mld, double min_ratio, double target_lexfreq, bool cs,
					 int min_df, double confidence) {

  int    cnt             = 0;
  int    distr_count     = 0;
  int    target_freq     = 0;
  int    answer_freq     = 0;
  double prob            = 0.0;
  double target_distprob = 0.0;
  double answer_prob     = 0.0;
  //double target_lexfreq  = 0.0; // now parameter

  cnt = vd->size();
  distr_count = vd->totalSize();

  Timbl::ValueDistribution::dist_iterator it = vd->begin();
  std::map<std::string,int>::iterator tvsfi;
  std::map<std::string,int>::iterator wfi;
  double factor = 0.0;

  // NB some of the test are outside this loop (max_distr, in_distr)
  while ( it != vd->end() ) { // loop over distribution

    std::string tvs  = it->second->Value()->Name(); // element in distribution
    double      wght = it->second->Weight(); // frequency of distr. element
    // LD shortcut, if stringlength differs more than mld, we don't need to try
    int         ld   = lev_distance( target, tvs, mld, cs ); // LD with target (word in text)

    // If the ld of the word is less than the maximum ld (mld parameter),
    // we include the result in our output.
    //
    if (
	( ld > 0 ) &&
	( ld <= mld ) &&
	( (min_df == 0) || (wght >= min_df) )
	) {
      //
      // So here we check frequency of element from the distr. with
      // frequency of the target. As parameter, we prolly already know it?
      // This is to determine if, even if target is a dictionary word,
      // it could be a misspelt word. Calculate the ratio between the
      // element from the distribution and the frequency of the target.
      // (PJB: should we not check target DISTR frequency?)
      int    tvs_lf = 0;
      double factor = 0.0;
      wfi = wfreqs.find( tvs );
      // if we find distr. element in lexicon, and target is in lexicon
      if ( (wfi != wfreqs.end()) && (target_lexfreq > 0) ) {
	tvs_lf =  (int)(*wfi).second;
	factor = tvs_lf / target_lexfreq;
      }
      //
      // std::cerr << tvs << "-" << tvs_lf << "/" << factor << std::endl;
      // If the target is not found (unknown words), we have no
      // ratio, we store the distribution element (possible correction).
      // IF we have a ratio, and it is >= min_ratio, we also store
      // this possible correction.
      //
      if ( (target_lexfreq == 0) || (factor >= min_ratio) ) {
	//
	distr_elem* d = new distr_elem();
	d->name = tvs;
	d->freq = wght; //was ld. Freq not so useful, can be low for even
	d->ld = ld;     // a correct to the point classification
	tvsfi = wfreqs.find( tvs );
	if ( tvsfi == wfreqs.end() ) {
	  d->lexfreq = 0;
	} else {
	  d->lexfreq = (double)(*tvsfi).second;
	}
	
	distr_vec.push_back( d );
      } // factor>=min_ratio
    }
    
    ++it;
  }
}

// test version paired with tcorrect.
// Elements are added distr_vec
//
void tdistr_spelcorr( const Timbl::ValueDistribution *vd, const std::string& target, std::map<std::string,int>& wfreqs,
					 std::vector<distr_elem*>& distr_vec, int mld, bool cs,	int min_df, double confidence) {

  int    cnt             = 0;
  int    distr_count     = 0;
  int    target_freq     = 0;
  int    answer_freq     = 0;
  double prob            = 0.0;
  double target_distprob = 0.0;
  double answer_prob     = 0.0;
  //double target_lexfreq  = 0.0; // now parameter

  cnt = vd->size();
  distr_count = vd->totalSize();

  Timbl::ValueDistribution::dist_iterator it = vd->begin();
  std::map<std::string,int>::iterator tvsfi;
  std::map<std::string,int>::iterator wfi;
  double factor = 0.0;

  // NB some of the test are outside this loop (max_distr, in_distr)
  while ( it != vd->end() ) { // loop over distribution

    std::string tvs  = it->second->Value()->Name(); // element in distribution
    double      wght = it->second->Weight(); // frequency of distr. element
	// LD shortcut, if stringlength differs more than mld, we don't need to try
    int         ld   = lev_distance( target, tvs, mld, cs ); // LD with target (word in text)

	// Checks if distribution value is to be included here.
	// Levenshtein within bounds, or ld:0 to ignore
	//
    if ( ((ld > 0) && (ld <= mld)) || (mld == 0) ) {
	  // min_df is minimum distribution frequency of token in distr.
	  if ( (min_df == 0) || (wght >= min_df) ) { // min_df==0, ignore, else check
		distr_elem* d = new distr_elem();
		d->name = tvs;
		d->freq = wght; //was ld. Freq not so useful, can be low for even
		d->ld = ld;     // a correct, to the point, classification
		tvsfi = wfreqs.find( tvs );
		if ( tvsfi == wfreqs.end() ) {
		  d->lexfreq = 0;
		} else {
		  d->lexfreq = (double)(*tvsfi).second;
		}
		distr_vec.push_back( d );
	  } // min_df
    } // ld

    ++it;
  }
}

// Spellings correction.
//
/*
1. Meenemen van lexicale frequenties in Wopr Correct. We hadden het daar
al over; je neemt de frequenties mee uit het trainingset-lexicon en
geeft die mee in de output - of doet er zelfs al filtering op. Daarnaast
vraag ik me af of naast OF in plaats van een frequentiedrempel, er ook
een drempel gezet kan worden op de distributiegrootte of de entropie.
Dus: als entropy > 5, of als distributie > 100, of als frequentie-ratio
> 100, onderdruk dan de gesuggereerde correctie. In feite heb je hiermee
drie parameters. Zou je eens kunnen bedenken of dit in te bouwen is?
*/
#ifdef TIMBL
int correct( Logfile& l, Config& c ) {
  l.log( "correct" );
  const std::string& filename         = c.get_value( "filename" );
  std::string        dirname          = c.get_value( "dir", "" );
  std::string        dirmatch         = c.get_value( "dirmatch", ".*" );
  const std::string& ibasefile        = c.get_value( "ibasefile" );
  const std::string& lexicon_filename = c.get_value( "lexicon" );
  const std::string& counts_filename  = c.get_value( "counts" );
  const std::string& trigger_filename = c.get_value( "triggerfile", "" );
  const std::string& timbl            = c.get_value( "timbl" );
  const int          hapax            = my_stoi( c.get_value( "hpx", "0" ));
  const int          mode             = my_stoi( c.get_value( "mode", "0" )); // mode:0 is windowed, mode:1 is plain text
  const int          lc               = my_stoi( c.get_value( "lc", "2" ));
  const int          rc               = my_stoi( c.get_value( "rc", "0" ));
  std::string        id               = c.get_value( "id", to_str(getpid()) );
  //std::string        output_filename  = filename + id + ".sc";
  std::string        pre_s            = c.get_value( "pre_s", "<s>" );
  std::string        suf_s            = c.get_value( "suf_s", "</s>" );
  // minimum word length (guess added if > mwl)
  int                mwl              = my_stoi( c.get_value( "mwl", "5" ) );
  // maximum levenshtein distance (guess added if <= mld)
  int                mld              = my_stoi( c.get_value( "mld", "1" ) );
  // max entropy (guess added if <= max_entropy)
  int                max_ent          = my_stoi( c.get_value( "max_ent", "5" ) );
  // maximum distributie (guess added if <= max_distr)
  int                max_distr        = my_stoi( c.get_value( "max_distr", "10" ));
  // ratio target_lexfreq:tvs_lexfreq
  double             min_ratio        = my_stod( c.get_value( "min_ratio", "0" ));
  // maximum target frequency (word under scrutiny is not in dist or (<=) very low freq)
  int                max_tf           = my_stoi( c.get_value( "max_tf", "1" )); //for confusibles
  // minimum frequency of words in the distribution to be included, 0 is ignore
  int                min_df           = my_stoi( c.get_value( "min_df", "0" ));
  int                skip             = 0;
  bool               cs               = my_stoi( c.get_value( "cs", "1" )) == 1; //case insensitive levenshtein cs:0
  bool               typo_only        = my_stoi( c.get_value( "typos", "0" )) == 1;// typos only
  // Ratio between top-1 frequency and sum of rest of distribution frequencies
  double             confidence      = my_stod( c.get_value( "confidence", "0" ));
  // Minimum max-depth of timbl answer
  int                min_md           = my_stoi( c.get_value( "min_md", "0" )); //0 is >=0, is allow all

  Timbl::TimblAPI   *My_Experiment;
  std::string        distrib;
  std::vector<std::string> distribution;
  std::string        result;
  double             distance;

  // No slash at end of dirname.
  //
  if ( (dirname != "") && (dirname.substr(dirname.length()-1, 1) == "/") ) {
    dirname = dirname.substr(0, dirname.length()-1);
  }

  l.inc_prefix();
  if ( dirname != "" ) {
    l.log( "dir:             "+dirname );
    l.log( "dirmatch:        "+dirmatch );
  }
  //l.log( "filename:   "+filename );
  l.log( "ibasefile:  "+ibasefile );
  l.log( "lexicon:    "+lexicon_filename );
  l.log( "counts:     "+counts_filename );
  l.log( "triggerfile:"+trigger_filename );
  l.log( "timbl:      "+timbl );
  l.log( "id:         "+id );
  l.log( "hapax:      "+to_str(hapax) );
  l.log( "mode:       "+to_str(mode) );
  l.log( "lc:         "+to_str(lc) ); // left context size for windowing
  l.log( "rc:         "+to_str(rc) ); // right context size for windowing
  l.log( "mwl:        "+to_str(mwl) );
  l.log( "mld:        "+to_str(mld) );
  l.log( "max_ent:    "+to_str(max_ent) );
  l.log( "max_distr:  "+to_str(max_distr) );
  l.log( "min_ratio:  "+to_str(min_ratio) );
  l.log( "max_tf:     "+to_str(max_tf) );
  l.log( "min_df:     "+to_str(min_df) );
  l.log( "cs:         "+to_str(cs) );
  l.log( "confidence  "+to_str(confidence) );
  l.log( "min_md      "+to_str(min_md) );
  //l.log( "OUTPUT:     "+output_filename );
  l.dec_prefix();

  // Timbl
  if ( ! file_exists(l, c, ibasefile) ) {
    l.log( "ERROR: cannot read "+ibasefile+"." );
    return -1;
  }

  // One file, as before, or the whole globbed dir. Check if
  // they exists.
  //
  std::vector<std::string> filenames;
  std::vector<std::string>::iterator fi;
  if ( dirname == "" ) {
    filenames.push_back( filename );
  } else {
    get_dir( dirname, filenames, dirmatch );
  }
  l.log( "Processing "+to_str(filenames.size())+" files." );
  size_t numfiles = filenames.size();
  if ( numfiles == 0 ) {
    l.log( "No files found. Skipping." );
    return 0;
  }

  if ( contains_id(filenames[0], id) == true ) {
    id = "";
  } else {
    id = "_"+id;
  }

  for ( fi = filenames.begin(); fi != filenames.end(); fi++ ) {
    std::string a_file = *fi;
    std::string output_filename  = a_file + id + ".sc";

    if (file_exists(l,c,output_filename)) {
      //l.log( "Output for "+a_file+" exists, removing from list." );
      --numfiles;
    }
  }
  if ( numfiles == 0 ) {
    l.log( "All output files already exists, skipping." );
    return 0;
  }

#if defined(HAVE_TR1_UNORDERED_SET)
  std::tr1::unordered_set<std::string> triggers;
#else
  std::unordered_set<std::string> triggers;
#endif

  int trigger_count = 0;
  if ( trigger_filename != "" ) {
	std::ifstream file_triggers( trigger_filename.c_str() );
	if ( ! file_triggers ) {
	  l.log( "NOTICE: cannot load specified triggerfile." );
	  //return -1;
	} else {
	  l.log( "Reading triggers." );
	  std::string a_word;
	  while ( file_triggers >> a_word ) {
		triggers.insert(a_word);
		++trigger_count;
	  }
	  
	  file_triggers.close();
	  l.log( "Read triggers (total_count="+to_str(trigger_count)+")." );
	}
  }

  try {
    My_Experiment = new Timbl::TimblAPI( timbl );
    if ( ! My_Experiment->Valid() ) {
      l.log( "Timbl Experiment is not valid." );
      return 1;
    }
    (void)My_Experiment->GetInstanceBase( ibasefile );
    if ( ! My_Experiment->Valid() ) {
      l.log( "Timbl Experiment is not valid." );
      return 1;
    }
  } catch (...) {
    l.log( "Cannot create Timbl Experiment..." );
    return 1;
  }
  l.log( "Instance base loaded." );

  // Load lexicon. NB: hapaxed lexicon is different? Or add HAPAX entry?
  //
  int wfreq;
  unsigned long total_count = 0;
  unsigned long N_1 = 0; // Count for p0 estimate.
  unsigned long hpx_entries = 0;
  std::map<std::string,int> wfreqs; // whole lexicon
  std::map<std::string,int> hpxfreqs; // hapaxed list
  std::ifstream file_lexicon( lexicon_filename.c_str() );
  if ( ! file_lexicon ) {
    l.log( "NOTICE: cannot load lexicon file." );
    //return -1;
  } else {
    // Read the lexicon with word frequencies.
    // We need a hash with frequence - countOfFrequency, ffreqs.
    //
    l.log( "Reading lexicon." );
    std::string a_word;
    while ( file_lexicon >> a_word >> wfreq ) {
      wfreqs[a_word] = wfreq;
      total_count += wfreq;
      if ( wfreq == 1 ) {
		++N_1;
      }
	  if ( wfreq > hapax ) {
		hpxfreqs[a_word] = wfreq;
		++hpx_entries;
	  }
    }
    file_lexicon.close();
    l.log( "Read lexicon (total_count="+to_str(total_count)+")." );
  }

  // If we want smoothed counts, we need this file...
  // Make mapping <int, double> from c to c* ?
  //
  std::map<int,double> c_stars;
  int Nc0;
  double Nc1; // this is c*
  int count;
  std::ifstream file_counts( counts_filename.c_str() );
  if ( ! file_counts ) {
    l.log( "NOTICE: cannot read counts file, no smoothing will be applied." );
  } else {
    while( file_counts >> count >> Nc0 >> Nc1 ) {
      c_stars[count] = Nc1;
    }
    file_counts.close();
  }

  // The P(new_word) according to GoodTuring-3.pdf
  // We need the filename.cnt for this, because we really need to
  // discount the rest if we assign probability to the unseen words.
  //
  // We need to esitmate the total number of unseen words. Same as
  // vocab, i.e assume we saw half? Ratio of N_1 to total_count?
  //
  // We need to load .cnt file as well...
  //
  double p0 = 0.00001; // Arbitrary low probability for unknown words.
  if ( total_count > 0 ) { // Better estimate if we have a lexicon
    p0 = (double)N_1 / ((double)total_count * total_count);
  }
  //l.log( "P(new_particular) = " + to_str(p0) );

  for ( fi = filenames.begin(); fi != filenames.end(); fi++ ) {
    std::string a_file = *fi;
    std::string output_filename  = a_file + id + ".sc";

    l.log( "Processing: "+a_file );
    l.log( "OUTPUT:     "+output_filename );

    if ( file_exists(l,c,output_filename) ) {
      l.log( "OUTPUT files exist, not overwriting." );
      c.add_kv( "sc_file", output_filename );
      l.log( "SET sc_file to "+output_filename );
      continue;
    }

    l.inc_prefix();

    std::ifstream file_in( a_file.c_str() );
    if ( ! file_in ) {
      l.log( "ERROR: cannot load inputfile." );
      return -1;
    }
    std::ofstream file_out( output_filename.c_str(), std::ios::out );
    if ( ! file_out ) {
      l.log( "ERROR: cannot write output file." );
      return -1;
    }

    std::vector<std::string>::iterator vi;
    std::ostream_iterator<std::string> output( file_out, " " );

    std::string a_line;
	std::string classify_line;
    std::vector<std::string> results;
    std::vector<std::string> targets;
    std::vector<std::string>::iterator ri;
    const Timbl::ValueDistribution *vd;
    const Timbl::TargetValue *tv;
    std::vector<std::string> words;
    int correct = 0;
    int wrong   = 0;
    int correct_unknown = 0;
    int correct_distr = 0;

    // Recognise <s> or similar, reset pplx calculations.
    // Output results on </s> or similar.
    // Or a divisor which is not processed?
    //
    double sentence_prob      = 0.0;
    double sum_logprob        = 0.0;
    int    sentence_wordcount = 0;

	std::vector<std::string> cls;

    while( std::getline( file_in, classify_line )) {

	  cls.clear();
      words.clear();
      classify_line = trim( classify_line );

	  if ( mode == 1 ) { // plain text, must be windowed
	    window( classify_line, classify_line, lc, rc, (bool)false, 0, cls );
	  } else {
	    cls.push_back( classify_line );
	  }

	  for ( size_t i = 0; i < cls.size(); i++ ) {

		words.clear();
	    a_line = cls.at(i);
		a_line = trim( a_line );

		Tokenize( a_line, words, ' ' );

		if ( hapax > 0 ) {
		  int c = hapax_vector( words, hpxfreqs, hapax );
		  vector_to_string(words, a_line);
		  a_line = trim( a_line );
		  words.clear();
		  Tokenize( a_line, words, ' ' );
		}

		std::string target = words.at( words.size()-1 );

		++sentence_wordcount;

		// Is the target in the lexicon? We could calculate a smoothed
		// value here if we load the .cnt file too...
		//
		std::map<std::string,int>::iterator wfi = wfreqs.find( target );
		bool   target_unknown = false;
		bool   correct_answer = false;
		double target_lexfreq = 0.0;// should be double because smoothing
		double target_lexprob = 0.0;
		if ( wfi == wfreqs.end() ) {
		  target_unknown = true;
		} else {
		  target_lexfreq =  (int)(*wfi).second; // Take lexfreq, unless we smooth
		  std::map<int,double>::iterator cfi = c_stars.find( target_lexfreq );
		  if ( cfi != c_stars.end() ) { // We have a smoothed value, use it
			target_lexfreq = (double)(*cfi).second;
			//l.log( "smoothed_lexfreq = " + to_str(target_lexfreq) );
		  }
		  target_lexprob = (double)target_lexfreq / (double)total_count;
		}

		// If triggers, do naught if not a trigger
		//
		if ( trigger_count > 0 ) {
		  if ( triggers.find(target) == triggers.end() ) {
			file_out << a_line << " (" << target << ") "
					 << 0 << ' ' /*<< info << ' '*/ << 0 << ' ';
			file_out << 0 << ' ';
			file_out << 0 << " [ ";
			file_out << "]";
			file_out << std::endl;
			continue;
		  }
		}

		// What does Timbl think?
		// Do we change this answer to what is in the distr. (if it is?)
		//
		tv = My_Experiment->Classify( a_line, vd );
		if ( ! tv ) {
		  l.log( "ERROR: Timbl returned a classification error, aborting." );
		  break;
		}
		std::string answer = tv->Name();
		//l.log( "Answer: '" + answer + "' / '" + target + "'" );

		// Check match-depth, if too undeep, we are probably
		// unsure.
		//
		size_t md  = My_Experiment->matchDepth();
		bool   mal = My_Experiment->matchedAtLeaf();
		//l.log( "md/mal: " + to_str(md) + " / " + to_str(mal) );

		// we could skip here without calculating the rest of the stats if md < min_md
		// (but we don't)

		if ( target == answer ) {
		  ++correct;
		  correct_answer = true;
		} else {
		  ++wrong;
		}

		// Loop over distribution returned by Timbl.
		/*
		  Je hebt een WOPR, en je laat dat los op een nieuw stuk tekst. Voor ieder
		  woord in de tekst kijk je of het in de distributie zit. Zo NIET, dan
		  check je of er een woord in de distributie zit dat op een kleine
		  Levenshtein-afstand van het woord staat, bijvoorbeeld op afstand 1,
		  dus er is 1 insertie/deletie/transpositie verschil tussen de twee
		  woorden.
		*/
		// For spelling correction we look in the distro. If the word is not
		// there, we look at levenshtein distance 1, if there is a word
		// it could be the 'correction' of our target word.
		//
		// PJB: patterns containing spelling errors....
		//
		//
		Timbl::ValueDistribution::dist_iterator it = vd->begin();
		int cnt = 0;
		int distr_count = 0;
		int target_freq = 0;
		int answer_freq = 0;
		double prob            = 0.0;
		double target_distprob = 0.0;
		double answer_prob     = 0.0;
		double entropy         = 0.0;
		bool in_distr          = false;
		cnt = vd->size();
		distr_count = vd->totalSize();
		double answer_f = 0;

		// Check if target word is in the distribution.
		//
		while ( it != vd->end() ) {
		  //const Timbl::TargetValue *tv = it->second->Value();

		  std::string tvs  = it->second->Value()->Name();
		  double      wght = it->second->Weight();

		  // Prob. of this item in distribution.
		  //
		  prob     = (double)wght / (double)distr_count;
		  entropy -= ( prob * log2(prob) );

		  if ( tvs == target ) { // The correct answer was in the distribution!
			target_freq = wght;
			in_distr = true;
			if ( correct_answer == false ) {
			  ++correct_distr;
			  --wrong; // direct answer wrong, but right in distr. compensate count
			}
		  }
		  if ( tvs == answer ) { // Get the frequency to be able to calculate confidence
			answer_f = wght;
		  }

		  ++it;
		}
		target_distprob = (double)target_freq / (double)distr_count;

		// Confidence, after Skype call 20131101
		// frequency of top-1, the answer / sum(frequences rest of distribution)
		double the_confidence = -1; // -1 as shortcut to infinity
		if ( distr_count > 1 ) { // then there are more, with frequency, so we don't divide by 0
		  the_confidence = answer_f / distr_count;
		}

		// If correct: if target in distr, we take that prob, else
		// the lexical prob.
		// Unknown words?
		//
		double logprob = 0.0;
		if ( target_freq > 0 ) { // Right answer was in distr.
		  logprob = log2( target_distprob );
		} else {
		  if ( ! target_unknown ) { // Wrong, we take lex prob if known target
			logprob = log2( target_lexprob ); // SMOOTHED here, see above
		  } else {
			//
			// What to do here? We have an 'unknown' target, i.e. not in the
			// lexicon.
			//
			logprob = log2( p0 /*0.0001*/ ); // Foei!
		  }
		}
		sum_logprob += logprob;

		//l.log( "Target: "+target+" target_lexfreq: "+to_str(target_lexfreq) );

		// I we didn't have the correct answer in the distro, we take ld=1
		// Skip words shorter than mwl.
		//
		std::vector<distr_elem*> distr_vec;
		if ( md >= (size_t)min_md ) {
		  if ((cnt <= max_distr) && (target.length() >= (size_t)mwl) && ((in_distr == false)||(target_freq<=max_tf)) && (entropy <= max_ent)) {
			if ( (typo_only && target_unknown) || ( ! typo_only) ) {
			  if ( (the_confidence >= confidence) || ( the_confidence < 0 ) ) {
				distr_spelcorr( vd, target, wfreqs, distr_vec, mld, min_ratio, target_lexfreq, cs, min_df, confidence);
			  }
			}
		  }
		} else {
		  // not clearing means using the distribution as spelling corection anyway.
		  distr_vec.clear();
		}

		// Word logprob (ref. Antal's mail 21/11/08)
		// 2 ^ (-logprob(w))
		//
		double word_lp = pow( 2, -logprob );

		// What do we want in the output file? Write the pattern and answer,
		// the logprob, followed by the entropy (of distr.), the size of the
		// distribution returned, and the top-10 (or less) of the distribution.
		//
		file_out << a_line << " (" << answer << ") "
				 << logprob << ' ' /*<< info << ' '*/ << entropy << ' ';
		file_out << word_lp << ' ';
		int cntr = 0;
		sort( distr_vec.begin(), distr_vec.end(), distr_elem_cmprev_ptr() ); //NB: cmprev (versus cmp)
		std::vector<distr_elem*>::const_iterator fi = distr_vec.begin();
		file_out << cnt << " [ ";
		while ( (fi != distr_vec.end()) && (--cntr != 0) ) {
		  file_out << (*fi)->name << ' ' << (double)((*fi)->freq) << ' '; // print LD or freq? old was LD, now freq
		  delete *fi;
		  fi++;
		}
		distr_vec.clear();
		file_out << "]";
		file_out << std::endl;

		// End of sentence (sort of)
		//
		if ( target == "</s>" ) {
		  l.log( " sum_logprob = " + to_str( sum_logprob) );
		  l.log( " sentence_wordcount = " + to_str( sentence_wordcount ) );
		  double foo  = sum_logprob / (double)sentence_wordcount;
		  double pplx = pow( 2, -foo );
		  l.log( " pplx = " + to_str( pplx ) );
		  sum_logprob = 0.0;
		  sentence_wordcount = 0;
		  l.log( "--" );
		}

	  } // i.cls loop

    } // while getline()

    if ( sentence_wordcount > 0 ) { // Overgebleven zooi (of alles).
      l.log( "sum_logprob = " + to_str( sum_logprob) );
      l.log( "sentence_wordcount = " + to_str( sentence_wordcount ) );
      double foo  = sum_logprob / (double)sentence_wordcount;
      double pplx = pow( 2, -foo );
      l.log( "pplx = " + to_str( pplx ) );
    }

    file_out.close();
    file_in.close();

    l.log( "Correct:       " + to_str(correct) );
    l.log( "Correct Distr: " + to_str(correct_distr) );
    int correct_total = correct_distr+correct;
    l.log( "Correct Total: " + to_str(correct_total) );
    l.log( "Wrong:         " + to_str(wrong) );
    if ( sentence_wordcount > 0 ) {
      l.log( "Cor.tot/total: " + to_str(correct_total / (double)sentence_wordcount) );
      l.log( "Correct/total: " + to_str(correct / (double)sentence_wordcount) );
    }

    c.add_kv( "sc_file", output_filename );
    l.log( "SET sc_file to "+output_filename );

    l.dec_prefix();

  }

  return 0;
}
#else
int correct( Logfile& l, Config& c ) {
  l.log( "No TIMBL support." );
  return -1;
}
#endif

// Test version of correct, PARAMETERS ARE DEFAULT "OFF"
//
#ifdef TIMBL
int tcorrect( Logfile& l, Config& c ) {
  l.log( "tcorrect" );
  const std::string& filename         = c.get_value( "filename" );
  std::string        dirname          = c.get_value( "dir", "" );
  std::string        dirmatch         = c.get_value( "dirmatch", ".*" );
  const std::string& ibasefile        = c.get_value( "ibasefile" );
  const std::string& lexicon_filename = c.get_value( "lexicon" );
  const std::string& counts_filename  = c.get_value( "counts" );
  const std::string& trigger_filename = c.get_value( "triggerfile", "" );
  const std::string& timbl            = c.get_value( "timbl" );
  const int          hapax            = my_stoi( c.get_value( "hpx", "0" ));
  const int          mode             = my_stoi( c.get_value( "mode", "0" )); // mode:0 is windowed, mode:1 is plain text
  const int          lc               = my_stoi( c.get_value( "lc", "2" ));
  const int          rc               = my_stoi( c.get_value( "rc", "0" ));
  std::string        id               = c.get_value( "id", to_str(getpid()) );
  //std::string        output_filename  = filename + id + ".sc";
  std::string        pre_s            = c.get_value( "pre_s", "<s>" );
  std::string        suf_s            = c.get_value( "suf_s", "</s>" );
  // minimum word length (guess added if > mwl), ignored if 0.
  int                mwl              = my_stoi( c.get_value( "mwl", "0" ) );
  // maximum levenshtein distance (guess added if <= mld), or if 0 (ignores setting)
  int                mld              = my_stoi( c.get_value( "mld", "0" ) ); 
  // max entropy (guess added if <= max_entropy), -1 to ignore
  double             max_ent          = my_stod( c.get_value( "max_ent", "-1" ) );
  // maximum distributie (guess added if <= max_distr), 0 to ignore
  int                max_distr        = my_stoi( c.get_value( "max_distr", "0" ));
  // minimum sum freqs distribution (guess added if >= min_dsum)
  int                min_dsum         = my_stoi( c.get_value( "min_dsum", "0" ));
  // minimum frequency of words in the distribution to be included, 0 is ignore
  int                min_df           = my_stoi( c.get_value( "min_df", "0" ));
  int                skip             = 0;
  bool               cs               = my_stoi( c.get_value( "cs", "1" )) == 1; //case insensitive levenshtein cs:0
  // Ratio between top-1 frequency and sum of rest of distribution frequencies
  double             confidence      = my_stod( c.get_value( "confidence", "0" ));
  // Minimum max-depth of timbl answer
  int                min_md           = my_stoi( c.get_value( "min_md", "0" )); //0 is >=0, is allow all

  Timbl::TimblAPI   *My_Experiment;
  std::string        distrib;
  std::vector<std::string> distribution;
  std::string        result;
  double             distance;

#if defined(HAVE_TR1_UNORDERED_SET)
  std::tr1::unordered_set<std::string> triggers;
#else
  std::unordered_set<std::string> triggers;
#endif

  // No slash at end of dirname.
  //
  if ( (dirname != "") && (dirname.substr(dirname.length()-1, 1) == "/") ) {
    dirname = dirname.substr(0, dirname.length()-1);
  }

  l.inc_prefix();
  if ( dirname != "" ) {
    l.log( "dir:             "+dirname );
    l.log( "dirmatch:        "+dirmatch );
  }
  //l.log( "filename:   "+filename );
  l.log( "ibasefile:  "+ibasefile );
  l.log( "lexicon:    "+lexicon_filename );
  l.log( "counts:     "+counts_filename );
  l.log( "triggerfile:"+trigger_filename );
  l.log( "timbl:      "+timbl );
  l.log( "id:         "+id );
  l.log( "hapax:      "+to_str(hapax) );
  l.log( "mode:       "+to_str(mode) );
  l.log( "lc:         "+to_str(lc) ); // left context size for windowing
  l.log( "rc:         "+to_str(rc) ); // right context size for windowing
  l.log( "mwl:        "+to_str(mwl) );
  l.log( "mld:        "+to_str(mld) );
  l.log( "max_ent:    "+to_str(max_ent) );
  l.log( "max_distr:  "+to_str(max_distr) );
  l.log( "min_dsum:   "+to_str(min_dsum) );
  l.log( "min_df:     "+to_str(min_df) );
  l.log( "cs:         "+to_str(cs) );
  l.log( "confidence  "+to_str(confidence) );
  l.log( "min_md      "+to_str(min_md) );
  //l.log( "OUTPUT:     "+output_filename );
  l.dec_prefix();

  // Timbl
  if ( ! file_exists(l, c, ibasefile) ) {
    l.log( "ERROR: cannot read "+ibasefile+"." );
    return -1;
  }

  // One file, as before, or the whole globbed dir. Check if
  // they exists.
  //
  std::vector<std::string> filenames;
  std::vector<std::string>::iterator fi;
  if ( dirname == "" ) {
    filenames.push_back( filename );
  } else {
    get_dir( dirname, filenames, dirmatch );
  }
  l.log( "Processing "+to_str(filenames.size())+" files." );
  size_t numfiles = filenames.size();
  if ( numfiles == 0 ) {
    l.log( "No files found. Skipping." );
    return 0;
  }

  if ( contains_id(filenames[0], id) == true ) {
    id = "";
  } else {
    id = "_"+id;
  }

  for ( fi = filenames.begin(); fi != filenames.end(); fi++ ) {
    std::string a_file = *fi;
    std::string output_filename  = a_file + id + ".sc";

    if (file_exists(l,c,output_filename)) {
      //l.log( "Output for "+a_file+" exists, removing from list." );
      --numfiles;
    }
  }
  if ( numfiles == 0 ) {
    l.log( "All output files already exists, skipping." );
    return 0;
  }

  int trigger_count = 0;
  if ( trigger_filename != "" ) {
	std::ifstream file_triggers( trigger_filename.c_str() );
	if ( ! file_triggers ) {
	  l.log( "NOTICE: cannot load specified triggerfile." );
	  //return -1;
	} else {
	  l.log( "Reading triggers." );
	  std::string a_word;
	  while ( file_triggers >> a_word ) {
		triggers.insert(a_word);
		++trigger_count;
	  }
	  
	  file_triggers.close();
	  l.log( "Read triggers (total_count="+to_str(trigger_count)+")." );
	}
  }

  try {
    My_Experiment = new Timbl::TimblAPI( timbl );
    if ( ! My_Experiment->Valid() ) {
      l.log( "Timbl Experiment is not valid." );
      return 1;
    }
    (void)My_Experiment->GetInstanceBase( ibasefile );
    if ( ! My_Experiment->Valid() ) {
      l.log( "Timbl Experiment is not valid." );
      return 1;
    }
  } catch (...) {
    l.log( "Cannot create Timbl Experiment..." );
    return 1;
  }
  l.log( "Instance base loaded." );

  // Load lexicon. NB: hapaxed lexicon is different? Or add HAPAX entry?
  //
  int wfreq;
  unsigned long total_count = 0;
  unsigned long N_1 = 0; // Count for p0 estimate.
  unsigned long hpx_entries = 0;
  std::map<std::string,int> wfreqs; // whole lexicon
  std::map<std::string,int> hpxfreqs; // hapaxed list
  std::ifstream file_lexicon( lexicon_filename.c_str() );
  if ( ! file_lexicon ) {
    l.log( "NOTICE: cannot load lexicon file." );
    //return -1;
  } else {
    // Read the lexicon with word frequencies.
    // We need a hash with frequence - countOfFrequency, ffreqs.
    //
    l.log( "Reading lexicon." );
    std::string a_word;
    while ( file_lexicon >> a_word >> wfreq ) {
      wfreqs[a_word] = wfreq;
      total_count += wfreq;
      if ( wfreq == 1 ) {
		++N_1;
      }
	  if ( wfreq > hapax ) {
		hpxfreqs[a_word] = wfreq;
		++hpx_entries;
	  }
    }
    file_lexicon.close();
    l.log( "Read lexicon (total_count="+to_str(total_count)+")." );
  }

  // If we want smoothed counts, we need this file...
  // Make mapping <int, double> from c to c* ?
  //
  std::map<int,double> c_stars;
  int Nc0;
  double Nc1; // this is c*
  int count;
  std::ifstream file_counts( counts_filename.c_str() );
  if ( ! file_counts ) {
    l.log( "NOTICE: cannot read counts file, no smoothing will be applied." );
  } else {
    while( file_counts >> count >> Nc0 >> Nc1 ) {
      c_stars[count] = Nc1;
    }
    file_counts.close();
  }

  // The P(new_word) according to GoodTuring-3.pdf
  // We need the filename.cnt for this, because we really need to
  // discount the rest if we assign probability to the unseen words.
  //
  // We need to esitmate the total number of unseen words. Same as
  // vocab, i.e assume we saw half? Ratio of N_1 to total_count?
  //
  // We need to load .cnt file as well...
  //
  double p0 = 0.00001; // Arbitrary low probability for unknown words.
  if ( total_count > 0 ) { // Better estimate if we have a lexicon
    p0 = (double)N_1 / ((double)total_count * total_count);
  }
  //l.log( "P(new_particular) = " + to_str(p0) );

  for ( fi = filenames.begin(); fi != filenames.end(); fi++ ) {
    std::string a_file = *fi;
    std::string output_filename  = a_file + id + ".sc";
    std::string outlog_filename  = a_file + id + ".lg"; //LOG

    l.log( "Processing: "+a_file );
    l.log( "OUTPUT:     "+output_filename );
    l.log( "OUTPUT:     "+outlog_filename );

    if ( file_exists(l,c,output_filename) ) {
      l.log( "OUTPUT files exist, not overwriting." );
      c.add_kv( "sc_file", output_filename );
      l.log( "SET sc_file to "+output_filename );
      continue;
    }

    l.inc_prefix();

    std::ifstream file_in( a_file.c_str() );
    if ( ! file_in ) {
      l.log( "ERROR: cannot load inputfile." );
      return -1;
    }
    std::ofstream file_out( output_filename.c_str(), std::ios::out );
    if ( ! file_out ) {
      l.log( "ERROR: cannot write output file." );
      return -1;
    }
    std::ofstream log_out( outlog_filename.c_str(), std::ios::out );
    if ( ! log_out ) {
      l.log( "ERROR: cannot write log file." );
      return -1;
    }

    std::vector<std::string>::iterator vi;
    std::ostream_iterator<std::string> output( file_out, " " );

    std::string a_line;
	std::string classify_line;
    std::vector<std::string> results;
    std::vector<std::string> targets;
    std::vector<std::string>::iterator ri;
    const Timbl::ValueDistribution *vd;
    const Timbl::TargetValue *tv;
    std::vector<std::string> words;
    int correct = 0;
    int wrong   = 0;
    int correct_unknown = 0;
    int correct_distr = 0;

    // Recognise <s> or similar, reset pplx calculations.
    // Output results on </s> or similar.
    // Or a divisor which is not processed?
    //
    double sentence_prob      = 0.0;
    double sum_logprob        = 0.0;
    int    sentence_wordcount = 0;

	std::vector<std::string> cls;

    while( std::getline( file_in, classify_line )) {

	  cls.clear();
      words.clear();
      classify_line = trim( classify_line );

	  if ( mode == 1 ) { // plain text, must be windowed
	    window( classify_line, classify_line, lc, rc, (bool)false, 0, cls );
	  } else {
	    cls.push_back( classify_line );
	  }

	  for ( size_t i = 0; i < cls.size(); i++ ) {

		words.clear();
	    a_line = cls.at(i);
		a_line = trim( a_line );

		Tokenize( a_line, words, ' ' );

		if ( hapax > 0 ) {
		  int c = hapax_vector( words, hpxfreqs, hapax );
		  vector_to_string(words, a_line);
		  a_line = trim( a_line );
		  words.clear();
		  Tokenize( a_line, words, ' ' );
		}

		// The answer we are after, last feature value in instance.
		std::string target = words.at( words.size()-1 );

		++sentence_wordcount;

		// Is the target in the lexicon? We could calculate a smoothed
		// value here if we load the .cnt file too...
		//
		std::map<std::string,int>::iterator wfi = wfreqs.find( target );
		bool   target_unknown = false;
		bool   correct_answer = false;
		double target_lexfreq = 0.0;// should be double because smoothing
		double target_lexprob = 0.0;
		if ( wfi == wfreqs.end() ) {
		  target_unknown = true;
		} else {
		  target_lexfreq =  (int)(*wfi).second; // Take lexfreq, unless we smooth
		  std::map<int,double>::iterator cfi = c_stars.find( target_lexfreq );
		  if ( cfi != c_stars.end() ) { // We have a smoothed value, use it
			target_lexfreq = (double)(*cfi).second;
			//l.log( "smoothed_lexfreq = " + to_str(target_lexfreq) );
		  }
		  target_lexprob = (double)target_lexfreq / (double)total_count;
		}

		// If triggers, do naught if not a trigger
		//
		if ( trigger_count > 0 ) {
		  if ( triggers.find(target) == triggers.end() ) {
			file_out << a_line << " (" << target << ") "
					 << 0 << ' ' /*<< info << ' '*/ << 0 << ' ';
			file_out << 0 << ' ';
			file_out << 0 << " [ ";
			file_out << "]";
			file_out << std::endl;
			continue;
		  }
		}

		// What does Timbl think?
		// Do we change this answer to what is in the distr. (if it is?)
		//
		tv = My_Experiment->Classify( a_line, vd );
		if ( ! tv ) {
		  l.log( "ERROR: Timbl returned a classification error, aborting." );
		  break;
		}
		std::string answer = tv->Name();
		//l.log( "Answer: '" + answer + "' / '" + target + "'" );

		// Check match-depth, if too undeep, we are probably
		// unsure.
		//
		size_t md  = My_Experiment->matchDepth();
		bool   mal = My_Experiment->matchedAtLeaf();
		//l.log( "md/mal: " + to_str(md) + " / " + to_str(mal) );

		// we could skip here without calculating the rest of the stats if md < min_md
		// (but we don't)

		if ( target == answer ) {
		  ++correct;
		  correct_answer = true;
		} else {
		  ++wrong;
		}

		// Loop over distribution returned by Timbl.
		/*
		  Je hebt een WOPR, en je laat dat los op een nieuw stuk tekst. Voor ieder
		  woord in de tekst kijk je of het in de distributie zit. Zo NIET, dan
		  check je of er een woord in de distributie zit dat op een kleine
		  Levenshtein-afstand van het woord staat, bijvoorbeeld op afstand 1,
		  dus er is 1 insertie/deletie/transpositie verschil tussen de twee
		  woorden.
		*/
		// For spelling correction we look in the distro. If the word is not
		// there, we look at levenshtein distance 1, if there is a word
		// it could be the 'correction' of our target word.
		//
		// PJB: patterns containing spelling errors....
		//
		//
		Timbl::ValueDistribution::dist_iterator it = vd->begin();
		int cnt = 0;
		int distr_count = 0;
		int target_freq = 0;
		int answer_freq = 0;
		double prob            = 0.0;
		double target_distprob = 0.0;
		double answer_prob     = 0.0;
		double entropy         = 0.0;
		bool in_distr          = false;
		cnt = vd->size();
		distr_count = vd->totalSize();
		double answer_f = 0;

		// Check if target word is in the distribution.
		//
		while ( it != vd->end() ) {
		  //const Timbl::TargetValue *tv = it->second->Value();

		  std::string tvs  = it->second->Value()->Name();
		  double      wght = it->second->Weight();

		  // Prob. of this item in distribution.
		  //
		  prob     = (double)wght / (double)distr_count;
		  entropy -= ( prob * log2(prob) );

		  if ( tvs == target ) { // The correct answer was in the distribution!
			target_freq = wght;
			in_distr = true;
			if ( correct_answer == false ) {
			  ++correct_distr;
			  --wrong; // direct answer wrong, but right in distr. compensate count
			}
		  }
		  if ( tvs == answer ) { // Get the frequency to be able to calculate confidence
			answer_f = wght;
		  }

		  ++it;
		}
		target_distprob = (double)target_freq / (double)distr_count;

		// Confidence, after Skype call 20131203
		// frequency of top-1 (the answer) / sum(all frequencies), 0:50 is 50-50.
		double the_confidence = -1; // -1 as shortcut to infinity
		if ( distr_count > 0 ) { 
		  the_confidence = answer_f / distr_count;
		}

		// If correct: if target in distr, we take that prob, else
		// the lexical prob.
		// Unknown words?
		//
		double logprob = 0.0;
		if ( target_freq > 0 ) { // Right answer was in distr.
		  logprob = log2( target_distprob );
		} else {
		  if ( ! target_unknown ) { // Wrong, we take lex prob if known target
			logprob = log2( target_lexprob ); // SMOOTHED here, see above
		  } else {
			//
			// What to do here? We have an 'unknown' target, i.e. not in the
			// lexicon.
			//
			logprob = log2( p0 /*0.0001*/ ); // Foei!
		  }
		}
		sum_logprob += logprob;

		//l.log( "Target: "+target+" target_lexfreq: "+to_str(target_lexfreq) );

		// I we didn't have the correct answer in the distro, we take ld=1
		//
		std::vector<distr_elem*> distr_vec;
		// Several conditions, AND
		log_out << a_line << std::endl;
		if ( distr_count >= min_dsum ) {
		  log_out << "dsum [" << distr_count << "] >= min_dsum [" << min_dsum << "]" << std::endl;
		  if ( md >= (size_t)min_md ) { // match depth ok?
		    log_out << "md [" << md << "] >= min_md [" << min_md << "]" << std::endl;
		    if ( (the_confidence >= confidence) || ( the_confidence < 0 ) ) { // confidence OK?
		      log_out << "the_confidence [" << the_confidence << "] >= confidence [" << confidence << "] or the_confidence < 0 " << std::endl;
		      if ( (cnt < max_distr) || (max_distr == 0) ) {//size of distibution ok, 0 is fall thru
			log_out << "cnt [" << cnt << "] < max_distr [" << max_distr << "] or max_distr == 0" << std::endl;
			if ( (entropy < max_ent) || (max_ent < 0) ) { // entropy OK, -1 is fall thru
			  log_out << "entropy [" <<  entropy << "] < max_ent [" << max_ent << "] or max_ent < 0" << std::endl;
			  tdistr_spelcorr( vd, target, wfreqs, distr_vec, mld, cs, min_df, confidence); // filter the distro, could return []
			  log_out << "tdistr size: " << distr_vec.size() << std::endl; // the number of answers returned.
			}
		      }
		    }
		  }
		} else {
		  // not clearing means using the distribution as spelling correction anyway!
		  distr_vec.clear();
		}
		log_out << std::endl;

		// Word logprob (ref. Antal's mail 21/11/08)
		// 2 ^ (-logprob(w))
		//
		double word_lp = pow( 2, -logprob );

		// What do we want in the output file? Write the pattern and answer,
		// the logprob, followed by the entropy (of distr.), the size of the
		// distribution returned, and the top-10 (or less) of the distribution.
		//
		file_out << a_line << " (" << answer << ") "
				 << logprob << ' ' /*<< info << ' '*/ << entropy << ' ';
		file_out << word_lp << ' ';
		int cntr = 0;
		sort( distr_vec.begin(), distr_vec.end(), distr_elem_cmprev_ptr() ); //NB: cmprev (versus cmp)
		std::vector<distr_elem*>::const_iterator fi = distr_vec.begin();
		file_out << cnt << " [ ";
		while ( (fi != distr_vec.end()) && (--cntr != 0) ) {
		  file_out << (*fi)->name << ' ' << (double)((*fi)->freq) << ' '; // print LD or freq? old was LD, now freq
		  delete *fi;
		  fi++;
		}
		distr_vec.clear();
		file_out << "]";
		file_out << std::endl;

		// End of sentence (sort of)
		//
		if ( target == "</s>" ) {
		  l.log( " sum_logprob = " + to_str( sum_logprob) );
		  l.log( " sentence_wordcount = " + to_str( sentence_wordcount ) );
		  double foo  = sum_logprob / (double)sentence_wordcount;
		  double pplx = pow( 2, -foo );
		  l.log( " pplx = " + to_str( pplx ) );
		  sum_logprob = 0.0;
		  sentence_wordcount = 0;
		  l.log( "--" );
		}

	  } // i.cls loop

    } // while getline()

    if ( sentence_wordcount > 0 ) { // Overgebleven zooi (of alles).
      l.log( "sum_logprob = " + to_str( sum_logprob) );
      l.log( "sentence_wordcount = " + to_str( sentence_wordcount ) );
      double foo  = sum_logprob / (double)sentence_wordcount;
      double pplx = pow( 2, -foo );
      l.log( "pplx = " + to_str( pplx ) );
    }

    log_out.close();
    file_out.close();
    file_in.close();

    l.log( "Correct:       " + to_str(correct) );
    l.log( "Correct Distr: " + to_str(correct_distr) );
    int correct_total = correct_distr+correct;
    l.log( "Correct Total: " + to_str(correct_total) );
    l.log( "Wrong:         " + to_str(wrong) );
    if ( sentence_wordcount > 0 ) {
      l.log( "Cor.tot/total: " + to_str(correct_total / (double)sentence_wordcount) );
      l.log( "Correct/total: " + to_str(correct / (double)sentence_wordcount) );
    }

    c.add_kv( "sc_file", output_filename );
    l.log( "SET sc_file to "+output_filename );

    l.dec_prefix();

  }

  return 0;
}
#else
int tcorrect( Logfile& l, Config& c ) {
  l.log( "No TIMBL support." );
  return -1;
}
#endif


// Spelling corrector server
//lw0196:test_pplxs pberck$ ../wopr -r server_sc -p ibasefile:OpenSub-english.train.txt.l2r0.hpx1_-a1+D.ibase,timbl:"-a1 +D",lexicon:OpenSub-english.train.txt.lex,verbose:1
//echo "man is _" | nc localhost 1984
//
// On charybdis:
//pberck@charybdis:/exp/pberck/wopr$ ./wopr -r server_sc -p ibasefile:/exp2/pberck/wopr_learning_curve/DUTCH-TWENTE-ILK.tok.1e6.l3r3.ibase,timbl:"-a1 +D",lexicon:/exp2/pberck/wopr_learning_curve/DUTCH-TWENTE-ILK.tok.1e6.lex,verbose:1,mwl:3
//echo "in dienst een gehad . _ hersenvliesontst" | nc localhost 1984
// geeft "hersenvliesonsteking" als antwoord.
//
#if defined(TIMBLSERVER) && defined(TIMBL)
int server_sc( Logfile& l, Config& c ) {
  l.log( "server spelling correction" );

  const std::string& timbl      = c.get_value( "timbl" );
  const std::string& ibasefile  = c.get_value( "ibasefile" );
  const std::string port        = c.get_value( "port", "1984" );
  const int verbose             = my_stoi( c.get_value( "verbose", "0" ));
  const int keep                = my_stoi( c.get_value( "keep", "0" ));
  const std::string& lexicon_filename = c.get_value( "lexicon" );
  const long cachesize          = my_stoi( c.get_value( "cs", "100000" ));

  int                mwl              = my_stoi( c.get_value( "mwl", "5" ) );
  // maximum levenshtein distance (guess added if <= mld)
  int                mld              = my_stoi( c.get_value( "mld", "1" ) );
  // max entropy (guess added if <= max_entropy)
  int                max_ent          = my_stoi( c.get_value( "max_ent", "5" ) );
  // maximum distributie (guess added if <= max_distr)
  int                max_distr        = my_stoi( c.get_value( "max_distr", "10" ));
  // ratio target_lexfreq:tvs_lexfreq
  double             min_ratio        = my_stod( c.get_value( "min_ratio", "0" ));
  const std::string empty        = c.get_value( "empty", "__EMPTY__" );
  const int hapax                   = my_stoi( c.get_value( "hpx", "0" ));

  l.inc_prefix();
  l.log( "ibasefile:  "+ibasefile );
  l.log( "port:       "+port );
  l.log( "keep:       "+to_str(keep) ); // keep conn. open after sending result
  l.log( "verbose:    "+to_str(verbose) ); // be verbose, or more verbose
  l.log( "timbl:      "+timbl ); // timbl settings
  l.log( "lexicon     "+lexicon_filename ); // the lexicon
  l.log( "hapax:      "+to_str(hapax) );
  l.log( "cache_size: "+to_str(cachesize) ); // size of the cache
  l.log( "mwl:        "+to_str(mwl) );
  l.log( "mld:        "+to_str(mld) );
  l.log( "max_ent:    "+to_str(max_ent) );
  l.log( "max_distr:  "+to_str(max_distr) );
  l.log( "min_ratio:  "+to_str(min_ratio) );
  l.dec_prefix();


  // To be implemented later (maybe)
  //
  int mode  = 0;
  int lc    = 0;
  int rc    = 0;

  char sep = '\t';

  // Load lexicon.
  //
  std::ifstream file_lexicon( lexicon_filename.c_str() );
  if ( ! file_lexicon ) {
    l.log( "ERROR: cannot load lexicon file." );
    return -1;
  }
  // Read the lexicon with word frequencies, freq > hapax.
  //
  l.log( "Reading lexicon." );
  std::string a_word;
  int wfreq;
  unsigned long total_count     = 0;
  unsigned long lex_entries     = 0;
  unsigned long hpx_entries     = 0;
  unsigned long N_1             = 0; // Count for p0 estimate.
  std::map<std::string,int> wfreqs; // whole lexicon
  std::map<std::string,int> hpxfreqs; // hapaxed list
  while( file_lexicon >> a_word >> wfreq ) {
    ++lex_entries;
    total_count += wfreq;
    wfreqs[a_word] = wfreq;
    if ( wfreq == 1 ) {
      ++N_1;
    }
    if ( wfreq > hapax ) {
      hpxfreqs[a_word] = wfreq;
      ++hpx_entries;
    }
  }
  file_lexicon.close();
  l.log( "Read lexicon, "+to_str(hpx_entries)+"/"+to_str(lex_entries)+" (total_count="+to_str(total_count)+")." );

  double p0 = 0.00001; // Arbitrary low probability for unknown words.
  if ( total_count > 0 ) { // Better estimate if we have a lexicon
    p0 = (double)N_1 / ((double)total_count * total_count);
  }

  std::string distrib;
  std::vector<std::string> distribution;
  std::string result;
  double distance;
  const Timbl::ValueDistribution *vd;
  const Timbl::TargetValue *tv;

  signal(SIGCHLD, SIG_IGN);
  volatile sig_atomic_t running = 1;

  try {
    Timbl::TimblAPI *My_Experiment = new Timbl::TimblAPI( timbl );
    (void)My_Experiment->GetInstanceBase( ibasefile );

    Sockets::ServerSocket server;

    if ( ! server.connect( port )) {
      l.log( "ERROR: cannot start server: "+server.getMessage() );
      return 1;
    }
    if ( ! server.listen(  ) < 0 ) {
      l.log( "ERROR: cannot listen. ");
      return 1;
    };

    while ( running ) {  // main accept() loop
      l.log( "Listening..." );

      Sockets::ServerSocket *newSock = new Sockets::ServerSocket();
      if ( !server.accept( *newSock ) ) {
	if( errno == EINTR ) {
	  continue;
	} else {
	  l.log( "ERROR: " + server.getMessage() );
	  return 1;
	}
      }
      if ( verbose > 0 ) {
	l.log( "Connection " + to_str(newSock->getSockId()) + "/"
	       + std::string(newSock->getClientName()) );
      }

      //http://beej.us/guide/bgipc/output/html/singlepage/bgipc.html
      int cpid = fork();
      if ( cpid == 0 ) { // child process

	Cache *cache = new Cache( cachesize ); // local cache

	std::string buf;

	bool connection_open = true;
	std::vector<std::string> cls; // classify lines
	std::vector<double> probs;

	while ( running && connection_open ) {

	  std::string tmp_buf;
	  newSock->read( tmp_buf );
	  tmp_buf = trim( tmp_buf, " \n\r" );

	  if ( tmp_buf == "_EXIT_" ) {
	    connection_open = false;
	    running = 0;
	    break;
	  }
	  if ( tmp_buf == "_CLOSE_" ) {
	    connection_open = false;
	    break;
	  }
	  if ( tmp_buf.length() == 0 ) {
	    connection_open = false;
	    break;
	  }
	  if ( verbose > 1 ) {
	    l.log( "|" + tmp_buf + "|" );
	  }

	  cls.clear();

	  std::string classify_line = tmp_buf;
	  std::string full_string = tmp_buf;

	  // Check the cache
	  //
	  std::string cache_ans = cache->get( full_string );
	  if ( cache_ans != "" ) {
	    if ( verbose > 2 ) {
	      l.log( "Found in cache." );
	    }
	    newSock->write(  cache_ans + '\n' );
	    continue;
	  }

	  // Sentence based, window here, classify all, etc.
	  // mode is 0, hardcoded. Valkuil uses mode:0
	  //
	  if ( mode == 1 ) {
	    window( classify_line, classify_line, lc, rc, (bool)false, 0, cls );
	  } else {
	    cls.push_back( classify_line );
	  }

	  // Loop over all lines.
	  //
	  std::vector<std::string> words;
	  probs.clear();
	  for ( size_t i = 0; i < cls.size(); i++ ) {

	    classify_line = cls.at(i);

	    words.clear();
	    Tokenize( classify_line, words, ' ' );

	    if ( hapax > 0 ) {
	      int c = hapax_vector( words, hpxfreqs, hapax );
	      std::string t;
	      vector_to_string(words, t);
	      classify_line = t;
	      if ( verbose > 1 ) {
		l.log( "|" + classify_line + "| hpx" );
	      }
	    }

	    // if we take target from a pre-non-hapaxed vector, we
	    // can hapax the whole sentence in the beginning and use
	    // that for the instances-without-target
	    //
	    std::string target = words.at( words.size()-1 );

	    // Is the target in the lexicon?
	    //
	    std::map<std::string,int>::iterator wfi = wfreqs.find( target );
	    bool   target_unknown = false;
	    bool   correct_answer = false;
	    double target_lexfreq = 0.0;
	    double target_lexprob = 0.0;
	    if ( wfi == wfreqs.end() ) {
	      target_unknown = true;
	    } else {
	      target_lexfreq =  (int)(*wfi).second; // Take lexfreq
	      target_lexprob = (double)target_lexfreq / (double)total_count;
	    }

	    tv = My_Experiment->Classify( classify_line, vd, distance );
	    if ( ! tv ) {
	      l.log("ERROR: Timbl returned a classification error, aborting.");
	      break;
	    }

	    // Check match-depth, if too undeep, we are probably
	    // unsure.
	    //
	    size_t md  = My_Experiment->matchDepth();
	    bool   mal = My_Experiment->matchedAtLeaf();

	    size_t      res_freq = tv->ValFreq();
	    std::string answer   = tv->Name();
	    if ( verbose > 1 ) {
	      l.log( "timbl("+classify_line+")="+answer );
	    }

	    if ( target == answer ) {
	      correct_answer = true;
	    }

	    int cnt         = 0;
	    int distr_count = 0;
	    int target_freq = 0;
	    int answer_freq = 0;

	    double prob            = 0.0;
	    double target_distprob = 0.0;
	    double answer_prob     = 0.0;
	    double entropy         = 0.0;
	    double sentence_prob   = 0.0;
	    double sum_logprob     = 0.0;

	    bool in_distr          = false;

	    cnt         = vd->size();
	    distr_count = vd->totalSize();

	    // Check if target word is in the distribution.
	    //
	    Timbl::ValueDistribution::dist_iterator it = vd->begin();
	    if ( cnt < max_distr ) {
	      while ( it != vd->end() ) {
		//const Timbl::TargetValue *tv = it->second->Value();

		std::string tvs  = it->second->Value()->Name();
		double      wght = it->second->Weight();

		if ( verbose > 1 ) {
		  l.log(tvs+": "+to_str(wght));
		}

		// Prob. of this item in distribution.
		//
		prob     = (double)wght / (double)distr_count;
		entropy -= ( prob * log2(prob) );

		if ( tvs == target ) { // The correct answer was in the distr.
		  target_freq = wght;
		  in_distr = true;
		}

		++it;
	      }
	    }
	    target_distprob = (double)target_freq / (double)distr_count;

	    // If correct: if target in distr, we take that prob, else
	    // the lexical prob.
	    // Unknown words?
	    //
	    double logprob = 0.0;
	    std::string info = "huh?";
	    if ( target_freq > 0 ) { // Right answer was in distr.
	      logprob = log2( target_distprob );
	      info = "target_distprob";
	    } else {
	      if ( ! target_unknown ) { // Wrong, we take lex prob if known target
		logprob = log2( target_lexprob );
		info = "target_lexprob";
	      } else {
		//
		// What to do here? We have an 'unknown' target, i.e. not in the
		// lexicon.
		//
		logprob = log2( p0 );
		info = "P(new_particular)";
	      }
	    }
	    sum_logprob += logprob;

	    // If we didn't have the correct answer in the distro, we take ld=1
	    // Skip words shorter than mwl.
	    //
	    it = vd->begin();
	    std::vector<distr_elem*> distr_vec;
	    std::map<std::string,int>::iterator tvsfi;
	    double factor = 0.0;

	    // if in_distr==true, we can look if att ld=1, and then at freq.factor!
	    if ( cnt < max_distr ) {
	      if ( (target.length() >= (size_t)mwl) && (in_distr == false) ) {
		while ( it != vd->end() ) {

		  // 20100111: freq voorspelde woord : te voorspellen woord > 1
		  //             uit de distr          target

		  std::string tvs  = it->second->Value()->Name();
		  double      wght = it->second->Weight();
		  int ld = lev_distance( target, tvs, mld, true );

		  if ( verbose > 1 ) {
		    l.log(tvs+": "+to_str(wght)+" ld: "+to_str(ld));
		  }

		  // If the ld of the word is less than the minimum,
		  // we include the result in our output.
		  //
		  if (
		      (entropy <= max_ent) &&
		      (cnt <= max_distr)   &&
		      ( ld <= mld )
		      ) {
		    //
		    // So here we check frequency of tvs from the distr. with
		    // frequency of the target.
		    //
		    int    tvs_lf = 0;
		    double factor = 0.0;
		    wfi = wfreqs.find( tvs );
		    if ( (wfi != wfreqs.end()) && (target_lexfreq > 0) ) {
		      tvs_lf =  (int)(*wfi).second;
		      factor = tvs_lf / target_lexfreq;
		    }
		    //
		    //l.log( tvs+"-"+to_str(tvs_lf)+"/"+to_str(factor) );
		    // If the target is not found (unknown words), we have no
		    // ratio, and we only use the other parameters, ie. this
		    // test falls through.
		    //
		    if ( (target_lexfreq == 0) || (factor >= min_ratio) ) {
		      //
		      distr_elem* d = new distr_elem();
		      d->name = tvs;
		      d->freq = ld;
		      tvsfi = wfreqs.find( tvs );
		      if ( tvsfi == wfreqs.end() ) {
				d->lexfreq = 0;
		      } else {
				d->lexfreq = (double)(*tvsfi).second;
		      }

		      distr_vec.push_back( d );
		    } // factor>min_ratio
		  }

		  ++it;
		}
	      }
	    }// PJB max_distr,

	    // Word logprob (ref. Antal's mail 21/11/08)
	    // 2 ^ (-logprob(w))
	    //
	    double word_lp = pow( 2, -logprob );
	    sort( distr_vec.begin(), distr_vec.end(), distr_elem_cmp_ptr() );

	    // Return all, seperated by sep (tab).

	    answer = "";
	    int cntr = -1;
	    std::vector<distr_elem*>::const_iterator fi = distr_vec.begin();
	    if ( distr_vec.empty() ) {
	      answer = empty;
	    } else {
	      while ( (fi != distr_vec.end()-1 ) && (--cntr != 0) ) {
		answer = answer + (*fi)->name + sep;
		delete *fi;
		fi++;
	      }
	      answer = answer + (*fi)->name;
	    }
	    cache->add( full_string, answer );
	    answer = answer + '\n';
	    newSock->write( answer );

	    distr_vec.clear();

	  } // i loop, over cls

	  connection_open = (keep == 1);
	  //connection_open = false;

	} // connection_open

        l.log( "connection closed." );
	if ( ! running ) {
	  // Rather crude, children with connexion keep working
	  // till exited/removed by system.
	  kill( getppid(), SIGTERM );
	}
	size_t ccs = cache->get_size();
	l.log( "Cache now: "+to_str(ccs)+"/"+to_str(cachesize)+" elements." );
	l.log( cache->stat() );
	_exit(0);

      } else if ( cpid == -1 ) { // fork failed
	l.log( "Error forking." );
	perror("fork");
	return(1);
      }
      delete newSock;

    }// running is true
  } //try
  catch ( const std::exception& e ) {
    l.log( "ERROR: exception caught." );
    std::cerr << e.what() << std::endl;
    return -1;
  }

  return 0;
}
#else
int server_sc( Logfile& l, Config& c ) {
  l.log( "Timbl support not built in." );
  return -1;
}
#endif

#if defined(TIMBLSERVER) && defined(TIMBL)
int server_sc_nf( Logfile& l, Config& c ) {
  l.log( "server spelling correction, non forking." );

  const std::string& timbl      = c.get_value( "timbl" );
  const std::string& ibasefile  = c.get_value( "ibasefile" );
  const std::string port        = c.get_value( "port", "1984" );
  const int verbose             = my_stoi( c.get_value( "verbose", "0" ));
  const int keep                = my_stoi( c.get_value( "keep", "0" ));
  const std::string& lexicon_filename = c.get_value( "lexicon" );

  int                mwl              = my_stoi( c.get_value( "mwl", "5" ) );
  // maximum levenshtein distance (guess added if <= mld)
  int                mld              = my_stoi( c.get_value( "mld", "1" ) );
  // max entropy (guess added if <= max_entropy)
  int                max_ent          = my_stoi( c.get_value( "max_ent", "5" ) );
  // maximum distributie (guess added if <= max_distr)
  int                max_distr        = my_stoi( c.get_value( "max_distr", "10" ));
  // ratio target_lexfreq:tvs_lexfreq
  double             min_ratio        = my_stod( c.get_value( "min_ratio", "0" ));
  const std::string empty        = c.get_value( "empty", "__EMPTY__" );

  const long cachesize            = my_stoi( c.get_value( "cs", "100000" ));

  l.inc_prefix();
  l.log( "ibasefile:  "+ibasefile );
  l.log( "port:       "+port );
  l.log( "keep:       "+to_str(keep) ); // keep conn. open after sending result
  l.log( "verbose:    "+to_str(verbose) ); // be verbose, or more verbose
  l.log( "timbl:      "+timbl ); // timbl settings
  l.log( "lexicon     "+lexicon_filename ); // the lexicon
  l.log( "cache_size: "+to_str(cachesize) ); // size of the cache
  l.log( "mwl:        "+to_str(mwl) );
  l.log( "mld:        "+to_str(mld) );
  l.log( "max_ent:    "+to_str(max_ent) );
  l.log( "max_distr:  "+to_str(max_distr) );
  l.log( "min_ratio:  "+to_str(min_ratio) );
  l.dec_prefix();

  // To be implemented later (maybe)
  //
  int hapax = 0;
  int mode  = 0;
  int lc    = 0;
  int rc    = 0;

  char sep = '\t';

  // Load lexicon.
  //
  std::ifstream file_lexicon( lexicon_filename.c_str() );
  if ( ! file_lexicon ) {
    l.log( "ERROR: cannot load lexicon file." );
    return -1;
  }
  // Read the lexicon with word frequencies, freq > hapax.
  //
  l.log( "Reading lexicon." );
  std::string a_word;
  int wfreq;
  unsigned long total_count     = 0;
  unsigned long lex_entries     = 0;
  unsigned long hpx_entries     = 0;
  unsigned long N_1             = 0; // Count for p0 estimate.
  std::map<std::string,int> wfreqs; // whole lexicon
  std::map<std::string,int> hpxfreqs; // hapaxed list
  while( file_lexicon >> a_word >> wfreq ) {
    ++lex_entries;
    total_count += wfreq;
    wfreqs[a_word] = wfreq;
    if ( wfreq == 1 ) {
      ++N_1;
    }
    if ( wfreq > hapax ) {
      hpxfreqs[a_word] = wfreq;
      ++hpx_entries;
    }
  }
  file_lexicon.close();
  l.log( "Read lexicon, "+to_str(hpx_entries)+"/"+to_str(lex_entries)+" (total_count="+to_str(total_count)+")." );

  double p0 = 0.00001; // Arbitrary low probability for unknown words.
  if ( total_count > 0 ) { // Better estimate if we have a lexicon
    p0 = (double)N_1 / ((double)total_count * total_count);
  }

  Cache *cache = new Cache( cachesize );

  std::string distrib;
  std::vector<std::string> distribution;
  std::string result;
  double distance;
  const Timbl::ValueDistribution *vd;
  const Timbl::TargetValue *tv;

  /*
    ------------ break ----------------
  */
  //  while ( true ) {  // old main accept() loop
  //l.log( "Starting server..." );

  try {
    Timbl::TimblAPI *My_Experiment = new Timbl::TimblAPI( timbl );
    (void)My_Experiment->GetInstanceBase( ibasefile );

    Sockets::ServerSocket server;

    if ( ! server.connect( port )) {
      l.log( "ERROR: cannot start server: "+server.getMessage() );
      return 1;
    }
    if ( ! server.listen(  ) < 0 ) {
      l.log( "ERROR: cannot listen. ");
      return 1;
    };

    while ( true ) {  // main accept() loop
      l.log( "Listening..." );

      Sockets::ServerSocket *newSock = new Sockets::ServerSocket();
      if ( !server.accept( *newSock ) ) {
		if( errno == EINTR ) {
		  continue;
		} else {
		  l.log( "ERROR: " + server.getMessage() );
		  return 1;
		}
      }
      if ( verbose > 0 ) {
		l.log( "Connection " + to_str(newSock->getSockId()) + "/"
			   + std::string(newSock->getClientName()) );
      }

      std::string buf;

      bool connection_open = true;
      std::vector<std::string> cls; // classify lines
      std::vector<double> probs;

      while ( connection_open ) {

		std::string tmp_buf;
		newSock->read( tmp_buf );
		tmp_buf = trim( tmp_buf, " \n\r" );

		if ( tmp_buf == "_CLOSE_" ) {
		  connection_open = false;
		  c.set_status(0);
		  return(0);
		}
		if ( tmp_buf == "_CLEAR_" ) {
		  //cache.clear();
		  //l.log( "Cache now: "+to_str(cache.size())+" elements." );
		  continue;
		}
		if ( tmp_buf.length() == 0 ) {
		  connection_open = false;
		  c.set_status(0);
		  break;
		}
		if ( verbose > 1 ) {
		  l.log( "|" + tmp_buf + "|" );
		}

		cls.clear();

		std::string classify_line = tmp_buf;

		// Sentence based, window here, classify all, etc.
		//
		if ( mode == 1 ) {
		  window( classify_line, classify_line, lc, rc, (bool)false, 0, cls );
		} else {
		  cls.push_back( classify_line );
		}

		// Loop over all lines.
		//
		std::vector<std::string> words;
		probs.clear();
		for ( size_t i = 0; i < cls.size(); i++ ) {

		  classify_line = cls.at(i);

		  // Check the cache
		  //
		  std::string cache_ans = cache->get( classify_line );
		  if ( cache_ans != "" ) {
			if ( verbose > 2 ) {
			  l.log( "Found in cache." );
			}
			newSock->write(  cache_ans + '\n' );
			continue;
		  }

		  words.clear();
		  Tokenize( classify_line, words, ' ' );

		  /*if ( hapax > 0 ) {
			int c = hapax_vector( words, hpxfreqs, hapax );
			std::string t;
			vector_to_string(words, t);
			classify_line = t;
			if ( verbose > 1 ) {
			l.log( "|" + classify_line + "| hpx" );
			}
			}*/

		  // if we take target from a pre-non-hapaxed vector, we
		  // can hapax the whole sentence in the beginning and use
		  // that for the instances-without-target
		  //
		  std::string target = words.at( words.size()-1 );

		  // Is the target in the lexicon?
		  //
	  std::map<std::string,int>::iterator wfi = wfreqs.find( target );
	  bool   target_unknown = false;
	  bool   correct_answer = false;
	  double target_lexfreq = 0.0;
	  double target_lexprob = 0.0;
	  if ( wfi == wfreqs.end() ) {
	    target_unknown = true;
	  } else {
	    target_lexfreq =  (int)(*wfi).second; // Take lexfreq
	    target_lexprob = (double)target_lexfreq / (double)total_count;
	  }

	  tv = My_Experiment->Classify( classify_line, vd, distance );
	  if ( ! tv ) {
	    l.log("ERROR: Timbl returned a classification error, aborting.");
	    break;
	  }

	  size_t      res_freq = tv->ValFreq();
	  std::string answer   = tv->Name();
	  if ( verbose > 1 ) {
	    l.log( "timbl("+classify_line+")="+answer );
	  }

	  if ( target == answer ) {
	    correct_answer = true;
	  }

	  int cnt         = 0;
	  int distr_count = 0;
	  int target_freq = 0;
	  int answer_freq = 0;

	  double prob            = 0.0;
	  double target_distprob = 0.0;
	  double answer_prob     = 0.0;
	  double entropy         = 0.0;
	  double sentence_prob   = 0.0;
	  double sum_logprob     = 0.0;

	  bool in_distr          = false;

	  cnt         = vd->size();
	  distr_count = vd->totalSize();

	  // Check if target word is in the distribution.
	  //
	  Timbl::ValueDistribution::dist_iterator it = vd->begin();
	  if ( cnt < max_distr ) {
	    while ( it != vd->end() ) {
	      //const Timbl::TargetValue *tv = it->second->Value();

	      std::string tvs  = it->second->Value()->Name();
	      double      wght = it->second->Weight();

	      if ( verbose > 1 ) {
			l.log(tvs+": "+to_str(wght));
	      }

	      // Prob. of this item in distribution.
	      //
	      prob     = (double)wght / (double)distr_count;
	      entropy -= ( prob * log2(prob) );

	      if ( tvs == target ) { // The correct answer was in the distr.
			target_freq = wght;
			in_distr = true;
	      }

	      ++it;
	    }
	  }
	  target_distprob = (double)target_freq / (double)distr_count;

	  // If correct: if target in distr, we take that prob, else
	  // the lexical prob.
	  // Unknown words?
	  //
	  double logprob = 0.0;
	  std::string info = "huh?";
	  if ( target_freq > 0 ) { // Right answer was in distr.
	    logprob = log2( target_distprob );
	    info = "target_distprob";
	  } else {
	    if ( ! target_unknown ) { // Wrong, we take lex prob if known target
	      logprob = log2( target_lexprob );
	      info = "target_lexprob";
	    } else {
	      //
	      // What to do here? We have an 'unknown' target, i.e. not in the
	      // lexicon.
	      //
	      logprob = log2( p0 );
	      info = "P(new_particular)";
	    }
	  }
	  sum_logprob += logprob;

	  // If we didn't have the correct answer in the distro, we take ld=1
	  // Skip words shorter than mwl.
	  //
	  it = vd->begin();
	  std::vector<distr_elem*> distr_vec;
	  std::map<std::string,int>::iterator tvsfi;
	  double factor = 0.0;

	  // if in_distr==true, we can look if att ld=1, and then at freq.factor!
	  if ( cnt < max_distr ) {
	    if ( (target.length() >= (size_t)mwl) && (in_distr == false) ) {
	      while ( it != vd->end() ) {

			// 20100111: freq voorspelde woord : te voorspellen woord > 1
			//             uit de distr          target

			std::string tvs  = it->second->Value()->Name();
			double      wght = it->second->Weight();
			int ld = lev_distance( target, tvs, mld, true );

			if ( verbose > 1 ) {
			  l.log(tvs+": "+to_str(wght)+" ld: "+to_str(ld));
			}

			// If the ld of the word is less than the minimum,
			// we include the result in our output.
			//
			if (
				(entropy <= max_ent) &&
				(cnt <= max_distr)   &&
				( ld <= mld )
				) {
			  //
			  // So here we check frequency of tvs from the distr. with
			  // frequency of the target.
			  //
			  int    tvs_lf = 0;
			  double factor = 0.0;
			  wfi = wfreqs.find( tvs );
			  if ( (wfi != wfreqs.end()) && (target_lexfreq > 0) ) {
				tvs_lf =  (int)(*wfi).second;
				factor = tvs_lf / target_lexfreq;
			  }
			  //
			  //l.log( tvs+"-"+to_str(tvs_lf)+"/"+to_str(factor) );
			  // If the target is not found (unknown words), we have no
			  // ratio, and we only use the other parameters, ie. this
			  // test falls through.
			  //
			  if ( (target_lexfreq == 0) || (factor >= min_ratio) ) {
				//
				distr_elem* d = new distr_elem();
				d->name = tvs;
				d->freq = ld;
				tvsfi = wfreqs.find( tvs );
				if ( tvsfi == wfreqs.end() ) {
				  d->lexfreq = 0;
				} else {
				  d->lexfreq = (double)(*tvsfi).second;
				}

				distr_vec.push_back( d );
			  } // factor>min_ratio
			}

			++it;
	      }
	    }
	  }
	  // Word logprob (ref. Antal's mail 21/11/08)
	  // 2 ^ (-logprob(w))
	  //
	  double word_lp = pow( 2, -logprob );
	  sort( distr_vec.begin(), distr_vec.end(), distr_elem_cmp_ptr() );

	  // Return all, seperated by sep (tab).

	  answer = "";
	  int cntr = -1;
	  std::vector<distr_elem*>::const_iterator fi = distr_vec.begin();
	  if ( distr_vec.empty() ) {
	    answer = empty;
	  } else {
	    while ( (fi != distr_vec.end()-1 ) && (--cntr != 0) ) {
	      answer = answer + (*fi)->name + sep;
	      delete *fi;
	      fi++;
	    }
	    answer = answer + (*fi)->name;
	  }

	  // Add.
	  //
	  cache->add( classify_line, answer );

	  answer = answer + '\n';
	  newSock->write( answer );

	  distr_vec.clear();

	} // i loop, over cls

	connection_open = (keep == 1);

      } // connection_open
      l.log( "connection closed." );
      delete newSock;

      size_t ccs = cache->get_size();
      l.log( "Cache now: "+to_str(ccs)+"/"+to_str(cachesize)+" elements." );
      l.log( cache->stat() );

    }//true
  } //try
  catch ( const std::exception& e ) {
    l.log( "ERROR: exception caught." );
    std::cerr << e.what() << std::endl;
    return -1;
  }
  ////  }// old true

  return 0;
}
#else
int server_sc_nf( Logfile& l, Config& c ) {
  l.log( "Timbl support not built in." );
  return -1;
}
#endif


// ---------------------------------------------------------------------------
// Multi correct
// ---------------------------------------------------------------------------

#ifdef TIMBL
int mcorrect( Logfile& l, Config& c ) {
  l.log( "correct" );
  const std::string& filename         = c.get_value( "filename" );
  const std::string& configfile       = c.get_value( "configfile" );
  std::string        dirname          = c.get_value( "dir", "" );
  std::string        dirmatch         = c.get_value( "dirmatch", ".*" );
  const std::string& lexicon_filename = c.get_value( "lexicon" );
  const std::string& counts_filename  = c.get_value( "counts" );
  const std::string& timbl            = c.get_value( "timbl" );
  const int          hapax            = my_stoi( c.get_value( "hpx", "0" ));
  const int          mode             = my_stoi( c.get_value( "mode", "0" )); // mode:0 is windowed, mode:1 is plain text
  const int          lc               = my_stoi( c.get_value( "lc", "2" ));
  const int          rc               = my_stoi( c.get_value( "rc", "0" ));
  std::string        id               = c.get_value( "id", to_str(getpid()) );
  //std::string        output_filename  = filename + id + ".sc";
  std::string        pre_s            = c.get_value( "pre_s", "<s>" );
  std::string        suf_s            = c.get_value( "suf_s", "</s>" );
  // minimum word length (guess added if > mwl)
  int                mwl              = my_stoi( c.get_value( "mwl", "5" ) );
  // maximum levenshtein distance (guess added if <= mld)
  int                mld              = my_stoi( c.get_value( "mld", "1" ) );
  // max entropy (guess added if <= max_entropy)
  int                max_ent          = my_stoi( c.get_value( "max_ent", "5" ) );
  // maximum distributie (guess added if <= max_distr)
  int                max_distr        = my_stoi( c.get_value( "max_distr", "10" ));
  // minimum sum freqs distribution (guess added if >= min_dsum)
  int                min_dsum         = my_stoi( c.get_value( "min_dsum", "0" ));
  // ratio target_lexfreq:tvs_lexfreq
  double             min_ratio        = my_stod( c.get_value( "min_ratio", "0" ));
  // maximum target frequency (word under scrutiny is not in dist or (<=) very low freq)
  int                max_tf           = my_stoi( c.get_value( "max_tf", "1" ));
  // minimum frequency of all words in the distribution
  int                min_df           = my_stoi( c.get_value( "min_df", "0" ));
  // offset for triggerpos, from the right, so 0 is the target position
  int                offset           = my_stoi( c.get_value( "offset", "0" ));
  int                skip             = 0;
  bool               cs               = my_stoi( c.get_value( "cs", "1" )) == 1; //case insensitive levenshtein cs:0
  bool               typo_only        = my_stoi( c.get_value( "typos", "0" )) == 1;// typos only
  bool               bl               = my_stoi( c.get_value( "bl", "0" )) == 1; //run baseline
  // Ratio between top-1 frequency and sum of distribution frequencies
  double             confidence      = my_stod( c.get_value( "confidence", "0" ));
  // Minimum max-depth of timbl answer
  int                min_md           = my_stoi( c.get_value( "min_md", "0" )); //0 is >=0, is allow all

  Timbl::TimblAPI   *My_Experiment;
  std::string        distrib;
  std::vector<std::string> distribution;
  std::string        result;
  double             distance;

  // No slash at end of dirname.
  //
  if ( (dirname != "") && (dirname.substr(dirname.length()-1, 1) == "/") ) {
    dirname = dirname.substr(0, dirname.length()-1);
  }

  l.inc_prefix();
  if ( dirname != "" ) {
    l.log( "dir:             "+dirname );
    l.log( "dirmatch:        "+dirmatch );
  }
  //l.log( "filename:   "+filename )
  l.log( "configfile: "+configfile );
  l.log( "lexicon:    "+lexicon_filename );
  l.log( "counts:     "+counts_filename );
  l.log( "timbl:      "+timbl );
  l.log( "id:         "+id );
  l.log( "hapax:      "+to_str(hapax) );
  l.log( "mode:       "+to_str(mode) );
  l.log( "lc:         "+to_str(lc) ); // left context size for windowing
  l.log( "rc:         "+to_str(rc) ); // right context size for windowing
  l.log( "mwl:        "+to_str(mwl) );
  l.log( "mld:        "+to_str(mld) );
  l.log( "max_ent:    "+to_str(max_ent) );
  l.log( "max_distr:  "+to_str(max_distr) );
  l.log( "min_dsum:   "+to_str(min_dsum) );
  l.log( "min_ratio:  "+to_str(min_ratio) );
  l.log( "max_tf:     "+to_str(max_tf) );
  l.log( "min_df:     "+to_str(min_df) );
  l.log( "offset:     "+to_str(offset) );
  l.log( "cs:         "+to_str(cs) );
  l.log( "bl:         "+to_str(bl) );
  l.log( "confidence  "+to_str(confidence) );
  l.log( "min_md      "+to_str(min_md) );
  //l.log( "OUTPUT:     "+output_filename );
  l.dec_prefix();

  // One file, as before, or the whole globbed dir. Check if
  // they exists.
  //
  std::vector<std::string> filenames;
  std::vector<std::string>::iterator fi;
  if ( dirname == "" ) {
    filenames.push_back( filename );
  } else {
    get_dir( dirname, filenames, dirmatch );
  }
  l.log( "Processing "+to_str(filenames.size())+" files." );
  size_t numfiles = filenames.size();
  if ( numfiles == 0 ) {
    l.log( "No files found. Skipping." );
    return 0;
  }

  if ( contains_id(filenames[0], id) == true ) {
    id = "";
  } else {
    id = "_"+id;
  }

  for ( fi = filenames.begin(); fi != filenames.end(); fi++ ) {
    std::string a_file = *fi;
    std::string output_filename  = a_file + id + ".sc";

    if (file_exists(l,c,output_filename)) {
      //l.log( "Output for "+a_file+" exists, removing from list." );
      --numfiles;
    }
  }
  if ( numfiles == 0 ) {
    l.log( "All output files already exists, skipping." );
    return 0;
  }

  // Load lexicon. NB: hapaxed lexicon is different? Or add HAPAX entry?
  //
  int wfreq;
  unsigned long total_count = 0;
  unsigned long N_1 = 0; // Count for p0 estimate.
  unsigned long hpx_entries = 0;
  std::map<std::string,int> wfreqs; // whole lexicon
  std::map<std::string,int> hpxfreqs; // hapaxed list
  std::ifstream file_lexicon( lexicon_filename.c_str() );
  if ( ! file_lexicon ) {
    l.log( "NOTICE: cannot load lexicon file." );
    //return -1;
  } else {
    // Read the lexicon with word frequencies.
    // We need a hash with frequence - countOfFrequency, ffreqs.
    //
    l.log( "Reading lexicon." );
    std::string a_word;
    while ( file_lexicon >> a_word >> wfreq ) {
      wfreqs[a_word] = wfreq;
      total_count += wfreq;
      if ( wfreq == 1 ) {
		++N_1;
      }
	  if ( wfreq > hapax ) {
		hpxfreqs[a_word] = wfreq;
		++hpx_entries;
	  }
    }
    file_lexicon.close();
    l.log( "Read lexicon (total_count="+to_str(total_count)+")." );
  }

  // Read configfile with ibase definitions
  // --------------------------------------

  std::vector<Expert*> exs;
  std::vector<Expert*>::iterator exi;
  std::map<std::string,Expert*> triggers; // reverse list, trigger to expert
  std::map<std::string,int> called; // counter

  if ( configfile != "" ) {
    l.log( "Reading classifiers." );
    std::string a_line;
    std::vector<std::string> words;
    std::ifstream ifs_config( configfile.c_str() );
    if ( ! ifs_config ) {
      l.log( "ERROR: cannot load configfile." );
      return -1;
    }
    while( std::getline( ifs_config, a_line )) {
      if ( a_line.length() == 0 ) {
	continue;
      }
      if ( a_line.at(0) == '#' ) {
	continue;
      }
      words.clear();
      Tokenize( a_line, words, ' ' );
      if ( words.size() < 2 ) {
	continue;
      }
      std::string ibf = words[0]; // name of ibasefile to read
      l.log( "EXPERT: "+ibf );
      Expert *e = new Expert("a", 1);
      e->set_ibasefile(ibf);
      e->set_timbl( timbl );
      e->init();
      int hi_freq = 0;
      for ( size_t i = 1; i < words.size(); i++ ) {
	std::string tr = words[i];
	triggers[ tr ] = e;
	called[ tr ] = 0;
	l.log( "TRIGGER: "+tr );
	// lookup if baseline
	if ( bl == true ) {
	  std::map<std::string,int>::iterator wfi = wfreqs.find( tr );
	  if ( wfi != wfreqs.end() ) {
	    int tr_freq = (int)(*wfi).second;
	    if ( tr_freq > hi_freq ) {
	      e->set_highest( tr );
	      e->set_highest_f( tr_freq );
	    }
	  }
	}
      }
    }
  }
  l.log( "Instance bases loaded." );

  // If we want smoothed counts, we need this file...
  // Make mapping <int, double> from c to c* ?
  //
  std::map<int,double> c_stars;
  int Nc0;
  double Nc1; // this is c*
  int count;
  std::ifstream file_counts( counts_filename.c_str() );
  if ( ! file_counts ) {
    l.log( "NOTICE: cannot read counts file, no smoothing will be applied." );
  } else {
    while( file_counts >> count >> Nc0 >> Nc1 ) {
      c_stars[count] = Nc1;
    }
    file_counts.close();
  }

  // The P(new_word) according to GoodTuring-3.pdf
  // We need the filename.cnt for this, because we really need to
  // discount the rest if we assign probability to the unseen words.
  //
  // We need to esitmate the total number of unseen words. Same as
  // vocab, i.e assume we saw half? Ratio of N_1 to total_count?
  //
  // We need to load .cnt file as well...
  //
  double p0 = 0.00001; // Arbitrary low probability for unknown words.
  if ( total_count > 0 ) { // Better estimate if we have a lexicon
    p0 = (double)N_1 / ((double)total_count * total_count);
  }
  //l.log( "P(new_particular) = " + to_str(p0) );

  for ( fi = filenames.begin(); fi != filenames.end(); fi++ ) {
    std::string a_file = *fi;
    std::string output_filename  = a_file + id + ".sc";

    l.log( "Processing: "+a_file );
    l.log( "OUTPUT:     "+output_filename );

    if ( file_exists(l,c,output_filename) ) {
      l.log( "OUTPUT files exist, not overwriting." );
      c.add_kv( "sc_file", output_filename );
      l.log( "SET sc_file to "+output_filename );
      continue;
    }

    l.inc_prefix();

    std::ifstream file_in( a_file.c_str() );
    if ( ! file_in ) {
      l.log( "ERROR: cannot load inputfile." );
      return -1;
    }
    std::ofstream file_out( output_filename.c_str(), std::ios::out );
    if ( ! file_out ) {
      l.log( "ERROR: cannot write output file." );
      return -1;
    }

    std::vector<std::string>::iterator vi;
    std::ostream_iterator<std::string> output( file_out, " " );

    std::string a_line;
	std::string classify_line;
    std::vector<std::string> results;
    std::vector<std::string> targets;
    std::vector<std::string>::iterator ri;
    const Timbl::ValueDistribution *vd;
    const Timbl::TargetValue *tv;
    std::vector<std::string> words;
    int correct = 0;
    int wrong   = 0;
    int correct_unknown = 0;
    int correct_distr = 0;

    // Recognise <s> or similar, reset pplx calculations.
    // Output results on </s> or similar.
    // Or a divisor which is not processed?
    //
    double sentence_prob      = 0.0;
    double sum_logprob        = 0.0;
    int    sentence_wordcount = 0;

	std::vector<std::string> cls;

	std::map<std::string, Expert*>::iterator tsi; //triggers
	Expert *e = NULL; // Expert used
	int pos;
	std::string trigger;

    while( std::getline( file_in, classify_line )) {

	  cls.clear();
      words.clear();
      classify_line = trim( classify_line );

	  if ( mode == 1 ) { // plain text, must be windowed
	    window( classify_line, classify_line, lc, rc, (bool)false, 0, cls );
	  } else {
	    cls.push_back( classify_line );
	  }

	  for ( size_t i = 0; i < cls.size(); i++ ) {

		words.clear();
	    a_line = cls.at(i);
		a_line = trim( a_line );

		Tokenize( a_line, words, ' ' );

		if ( hapax > 0 ) {
		  int c = hapax_vector( words, hpxfreqs, hapax );
		  vector_to_string(words, a_line);
		  a_line = trim( a_line );
		  words.clear();
		  Tokenize( a_line, words, ' ' );
		}

		std::string target = words.at( words.size()-1 );

		++sentence_wordcount;

		// Is the target in the lexicon? We could calculate a smoothed
		// value here if we load the .cnt file too...
		//
		std::map<std::string,int>::iterator wfi = wfreqs.find( target );
		bool   target_unknown = false;
		bool   correct_answer = false;
		double target_lexfreq = 0.0;// should be double because smoothing
		double target_lexprob = 0.0;
		if ( wfi == wfreqs.end() ) {
		  target_unknown = true;
		} else {
		  target_lexfreq =  (int)(*wfi).second; // Take lexfreq, unless we smooth
		  std::map<int,double>::iterator cfi = c_stars.find( target_lexfreq );
		  if ( cfi != c_stars.end() ) { // We have a smoothed value, use it
			target_lexfreq = (double)(*cfi).second;
			//l.log( "smoothed_lexfreq = " + to_str(target_lexfreq) );
		  }
		  target_lexprob = (double)target_lexfreq / (double)total_count;
		}

		// Do we Timbl, check triggers.
		pos     = words.size()-1-offset;
		pos     = (pos < 0) ? 0 : pos;
		trigger = words[pos];
		//l.log( "trigger: "+trigger);

		tsi = triggers.find( trigger );
		if ( tsi != triggers.end() ) {
		  e = (*tsi).second;
		} else { // the default classifier.
		  e = NULL;
		}

		if ( e == NULL ) {
		  // We print a dummy line, same format as normal experiments.
		  file_out << a_line << " (" << target << ") 0 0 1 1 [ ]\n";
		  continue;
		}

		if ( bl == true ) {
		  // We print a dummy line, same format as normal experiments.
		  file_out << a_line << " (" << e->get_highest() << ") 0 0 1 1 [ "+e->get_highest()+" "+to_str(e->get_highest_f())+" ]\n";
		  continue;
		}

		// What does Timbl think?
		// Do we change this answer to what is in the distr. (if it is?)
		//
		tv = e->My_Experiment->Classify( a_line, vd );
		e->call();
		called[trigger] += 1; //we know it exists
		if ( ! tv ) {
		  l.log( "ERROR: Timbl returned a classification error, aborting." );
		  l.log( "ERROR: "+a_line );
		  break;
		}
		std::string answer = tv->Name();
		//l.log( "Answer: '" + answer + "' / '" + target + "'" );

		// Check match-depth, if too undeep, we are probably
		// unsure.
		//
		size_t md  = 0; //My_Experiment->matchDepth();
		bool   mal = 0; //My_Experiment->matchedAtLeaf();

		if ( target == answer ) {
		  ++correct;
		  correct_answer = true;
		} else {
		  ++wrong;
		}

		// Loop over distribution returned by Timbl.
		/*
		  Je hebt een WOPR, en je laat dat los op een nieuw stuk tekst. Voor ieder
		  woord in de tekst kijk je of het in de distributie zit. Zo NIET, dan
		  check je of er een woord in de distributie zit dat op een kleine
		  Levenshtein-afstand van het woord staat, bijvoorbeeld op afstand 1,
		  dus er is 1 insertie/deletie/transpositie verschil tussen de twee
		  woorden.
		*/
		// For spelling correction we look in the distro. If the word is not
		// there, we look at levenshtein distance 1, if there is a word
		// it could be the 'correction' of our target word.
		//
		// PJB: patterns containing spelling errors....
		//
		//
		Timbl::ValueDistribution::dist_iterator it = vd->begin();
		int cnt = 0;
		int distr_count = 0;
		int target_freq = 0;
		int answer_freq = 0;
		double prob            = 0.0;
		double target_distprob = 0.0;
		double answer_prob     = 0.0;
		double entropy         = 0.0;
		bool in_distr          = false;
		cnt = vd->size();
		distr_count = vd->totalSize(); //sum of freqs, for min_dsum
		double answer_f = 0;

		// Check if target word is in the distribution.
		//
		while ( it != vd->end() ) {
		  //const Timbl::TargetValue *tv = it->second->Value();

		  std::string tvs  = it->second->Value()->Name();
		  double      wght = it->second->Weight();

		  // Prob. of this item in distribution.
		  //
		  prob     = (double)wght / (double)distr_count;
		  entropy -= ( prob * log2(prob) );

		  if ( tvs == target ) { // The correct answer was in the distribution!
			target_freq = wght;
			in_distr = true;
			if ( correct_answer == false ) {
			  ++correct_distr;
			  --wrong; // direct answer wrong, but right in distr. compensate count
			}
		  }
		  if ( tvs == answer ) { // Get the frequency to be able to calculate confidence
			answer_f = wght;
		  }

		  ++it;
		}
		target_distprob = (double)target_freq / (double)distr_count;

		// Confidence, after Skype call 20131101
		// frequency of top-1, the answer / sum(frequencies distribution)
		double the_confidence = -1; // -1 as shortcut to infinity
		if ( distr_count > 0 ) { // then there are more, with frequency, so we don't divide by 0
		  the_confidence = answer_f / distr_count;
		}

		// If correct: if target in distr, we take that prob, else
		// the lexical prob.
		// Unknown words?
		//
		double logprob = 0.0;
		if ( target_freq > 0 ) { // Right answer was in distr.
		  logprob = log2( target_distprob );
		} else {
		  if ( ! target_unknown ) { // Wrong, we take lex prob if known target
		    logprob = log2( target_lexprob ); // SMOOTHED here, see above
		  } else {
		    //
		    // What to do here? We have an 'unknown' target, i.e. not in the
		    // lexicon.
		    //
		    logprob = log2( p0 /*0.0001*/ ); // Foei!
		  }
		}
		sum_logprob += logprob;

		//l.log( "Target: "+target+" target_lexfreq: "+to_str(target_lexfreq) );

		// I we didn't have the correct answer in the distro, we take ld=1
		// Skip words shorter than mwl.
		//
		/* 2014-04-13
		std::vector<distr_elem*> distr_vec;
		if ( (cnt <= max_distr) && (target.length() >= mwl) && ((in_distr == false)||(target_freq<=max_tf)) && (entropy <= max_ent) ) {
		  if ( (typo_only && target_unknown) || ( ! typo_only) ) {
			distr_spelcorr( vd, target, wfreqs, distr_vec, mld, min_ratio, target_lexfreq, cs, min_df, confidence);
		  }
		}
		*/
		// from L1084
		std::vector<distr_elem*> distr_vec;
		if ( distr_count >= min_dsum ) {
		  if ( md >= (size_t)min_md ) {
		    if ((cnt <= max_distr) && (target.length() >= (size_t)mwl) && ((in_distr == false)||(target_freq<=max_tf)) && (entropy <= max_ent)) {
		      if ( (typo_only && target_unknown) || ( ! typo_only) ) {
			if ( (the_confidence >= confidence) || ( the_confidence < 0 ) ) {
			  distr_spelcorr( vd, target, wfreqs, distr_vec, mld, min_ratio, target_lexfreq, cs, min_df, confidence);
			}
		      }
		    }
		  }
		} else {
		  // not clearing means using the distribution as spelling corection anyway.
		  //distr_vec.clear();
		}


		// Word logprob (ref. Antal's mail 21/11/08)
		// 2 ^ (-logprob(w))
		//
		double word_lp = pow( 2, -logprob );

		// What do we want in the output file? Write the pattern and answer,
		// the logprob, followed by the entropy (of distr.), the size of the
		// distribution returned, and the top-10 (or less) of the distribution.
		//
		file_out << a_line << " (" << answer << ") "
			 << logprob << ' ' /*<< info << ' '*/ << entropy << ' ';
		file_out << word_lp << ' ';
		int cntr = 0;
		sort( distr_vec.begin(), distr_vec.end(), distr_elem_cmprev_ptr() ); //NB: cmprev (versus cmp)
		std::vector<distr_elem*>::const_iterator fi = distr_vec.begin();
		file_out << cnt << " [ ";
		while ( (fi != distr_vec.end()) && (--cntr != 0) ) {
		  file_out << (*fi)->name << ' ' << (double)((*fi)->freq) << ' '; // print LD or freq? old was LD, now freq
		  delete *fi;
		  fi++;
		}
		distr_vec.clear();
		file_out << "]";
		file_out << std::endl;

		// End of sentence (sort of)
		//
		if ( target == "</s>" ) {
		  l.log( " sum_logprob = " + to_str( sum_logprob) );
		  l.log( " sentence_wordcount = " + to_str( sentence_wordcount ) );
		  double foo  = sum_logprob / (double)sentence_wordcount;
		  double pplx = pow( 2, -foo );
		  l.log( " pplx = " + to_str( pplx ) );
		  sum_logprob = 0.0;
		  sentence_wordcount = 0;
		  l.log( "--" );
		}

	  } // i.cls loop

    } // while getline()

    if ( sentence_wordcount > 0 ) { // Overgebleven zooi (of alles).
      l.log( "sum_logprob = " + to_str( sum_logprob) );
      l.log( "sentence_wordcount = " + to_str( sentence_wordcount ) );
      double foo  = sum_logprob / (double)sentence_wordcount;
      double pplx = pow( 2, -foo );
      l.log( "pplx = " + to_str( pplx ) );
    }

    file_out.close();
    file_in.close();

    l.log( "Correct:       " + to_str(correct) );
    l.log( "Correct Distr: " + to_str(correct_distr) );
    int correct_total = correct_distr+correct;
    l.log( "Correct Total: " + to_str(correct_total) );
    l.log( "Wrong:         " + to_str(wrong) );
    if ( sentence_wordcount > 0 ) {
      l.log( "Cor.tot/total: " + to_str(correct_total / (double)sentence_wordcount) );
      l.log( "Correct/total: " + to_str(correct / (double)sentence_wordcount) );
    }

	// print call statistics for triggers.
	//
	std::map<std::string,int>::const_iterator ci = called.begin();
	while ( ci != called.end() ) {
	  l.log( ci->first+": "+to_str(ci->second) );
	  ci++;
	}

    c.add_kv( "sc_file", output_filename );
    l.log( "SET sc_file to "+output_filename );

    l.dec_prefix();

  }

  return 0;
}
#else
int mcorrect( Logfile& l, Config& c ) {
  l.log( "No TIMBL support." );
  return -1;
}
#endif

// 2015-03 confidence/mcorrect
#ifdef TIMBL
int cmcorrect( Logfile& l, Config& c ) {
  l.log( "cmcorrect" );
  const std::string& filename         = c.get_value( "filename" );
  const std::string& configfile       = c.get_value( "configfile" );
  std::string        dirname          = c.get_value( "dir", "" );
  std::string        dirmatch         = c.get_value( "dirmatch", ".*" );
  const std::string& lexicon_filename = c.get_value( "lexicon" );
  const std::string& counts_filename  = c.get_value( "counts" );
  const std::string& timbl            = c.get_value( "timbl" );
  const int          hapax            = my_stoi( c.get_value( "hpx", "0" ));
  const int          mode             = my_stoi( c.get_value( "mode", "0" )); // mode:0 is windowed, mode:1 is plain text
  const int          lc               = my_stoi( c.get_value( "lc", "2" ));
  const int          rc               = my_stoi( c.get_value( "rc", "0" ));
  std::string        id               = c.get_value( "id", to_str(getpid()) );
  //std::string        output_filename  = filename + id + ".sc";
  std::string        pre_s            = c.get_value( "pre_s", "<s>" );
  std::string        suf_s            = c.get_value( "suf_s", "</s>" );
  // offset for triggerpos, from the right, so 0 is the target position
  int                offset           = my_stoi( c.get_value( "offset", "0" ));
  int                skip             = 0;
  // Ratio between top-1 frequency and sum of distribution frequencies
  double             confidence      = my_stod( c.get_value( "confidence", "0" ));
  // Minimum max-depth of timbl answer
  int                min_md           = my_stoi( c.get_value( "min_md", "0" )); //0 is >=0, is allow all
  int                topn             = my_stoi( c.get_value( "topn", "0" ) );
    
  Timbl::TimblAPI   *My_Experiment;
  std::string        distrib;
  std::vector<std::string> distribution;
  std::string        result;
  double             distance;

  // No slash at end of dirname.
  //
  if ( (dirname != "") && (dirname.substr(dirname.length()-1, 1) == "/") ) {
    dirname = dirname.substr(0, dirname.length()-1);
  }

  l.inc_prefix();
  if ( dirname != "" ) {
    l.log( "dir:             "+dirname );
    l.log( "dirmatch:        "+dirmatch );
  }
  //l.log( "filename:   "+filename )
  l.log( "configfile: "+configfile );
  l.log( "lexicon:    "+lexicon_filename );
  l.log( "counts:     "+counts_filename );
  l.log( "timbl:      "+timbl );
  l.log( "id:         "+id );
  l.log( "hapax:      "+to_str(hapax) );
  l.log( "mode:       "+to_str(mode) );
  l.log( "topn:       "+to_str(topn) );
  l.log( "lc:         "+to_str(lc) ); // left context size for windowing
  l.log( "rc:         "+to_str(rc) ); // right context size for windowing
  l.log( "offset:     "+to_str(offset) );
  l.log( "confidence  "+to_str(confidence) );
  l.log( "min_md      "+to_str(min_md) );
  //l.log( "OUTPUT:     "+output_filename );
  l.dec_prefix();

  // One file, as before, or the whole globbed dir. Check if
  // they exists.
  //
  std::vector<std::string> filenames;
  std::vector<std::string>::iterator fi;
  if ( dirname == "" ) {
    filenames.push_back( filename );
  } else {
    get_dir( dirname, filenames, dirmatch );
  }
  l.log( "Processing "+to_str(filenames.size())+" files." );
  size_t numfiles = filenames.size();
  if ( numfiles == 0 ) {
    l.log( "No files found. Skipping." );
    return 0;
  }

  if ( contains_id(filenames[0], id) == true ) {
    id = "";
  } else {
    id = "_"+id;
  }

  for ( fi = filenames.begin(); fi != filenames.end(); fi++ ) {
    std::string a_file = *fi;
    std::string output_filename  = a_file + id + ".sc";
    std::string outlog_filename  = a_file + id + ".lg"; //LOG
    
    if (file_exists(l,c,output_filename)) {
      //l.log( "Output for "+a_file+" exists, removing from list." );
      --numfiles;
    }
  }
  if ( numfiles == 0 ) {
    l.log( "All output files already exists, skipping." );
    return 0;
  }

  // Load lexicon. NB: hapaxed lexicon is different? Or add HAPAX entry?
  //
  int wfreq;
  unsigned long total_count = 0;
  unsigned long N_1 = 0; // Count for p0 estimate.
  unsigned long hpx_entries = 0;
  std::map<std::string,int> wfreqs; // whole lexicon
  std::map<std::string,int> hpxfreqs; // hapaxed list
  std::ifstream file_lexicon( lexicon_filename.c_str() );
  if ( ! file_lexicon ) {
    l.log( "NOTICE: cannot load lexicon file." );
    //return -1;
  } else {
    // Read the lexicon with word frequencies.
    // We need a hash with frequence - countOfFrequency, ffreqs.
    //
    l.log( "Reading lexicon." );
    std::string a_word;
    while ( file_lexicon >> a_word >> wfreq ) {
      wfreqs[a_word] = wfreq;
      total_count += wfreq;
      if ( wfreq == 1 ) {
	++N_1;
      }
      if ( wfreq > hapax ) {
	hpxfreqs[a_word] = wfreq;
	++hpx_entries;
      }
    }
    file_lexicon.close();
    l.log( "Read lexicon (total_count="+to_str(total_count)+")." );
  }
  
  // Read configfile with ibase definitions
  // --------------------------------------
  
  std::vector<Expert*> exs;
  std::vector<Expert*>::iterator exi;
  std::map<std::string,Expert*> triggers; // reverse list, trigger to expert
  std::map<std::string,int> called; // counter
  
  if ( configfile != "" ) {
    l.log( "Reading classifiers." );
    std::string a_line;
    std::vector<std::string> words;
    std::ifstream ifs_config( configfile.c_str() );
    if ( ! ifs_config ) {
      l.log( "ERROR: cannot load configfile." );
      return -1;
    }
    while( std::getline( ifs_config, a_line )) {
      if ( a_line.length() == 0 ) {
	continue;
      }
      if ( a_line.at(0) == '#' ) {
	continue;
      }
      words.clear();
      Tokenize( a_line, words, ' ' );
      if ( words.size() < 2 ) {
	continue;
      }
      std::string ibf = words[0]; // name of ibasefile to read
      l.log( "EXPERT: "+ibf );
      Expert *e = new Expert("a", 1);
      e->set_ibasefile(ibf);
      e->set_timbl( timbl );
      e->init();
      int hi_freq = 0;
      for ( size_t i = 1; i < words.size(); i++ ) {
	std::string tr = words[i];
	triggers[ tr ] = e;
	called[ tr ] = 0;
	l.log( "TRIGGER: "+tr );
      }
    }
  }
  l.log( "Instance bases loaded." );

  // If we want smoothed counts, we need this file...
  // Make mapping <int, double> from c to c* ?
  //
  std::map<int,double> c_stars;
  int Nc0;
  double Nc1; // this is c*
  int count;
  std::ifstream file_counts( counts_filename.c_str() );
  if ( ! file_counts ) {
    l.log( "NOTICE: cannot read counts file, no smoothing will be applied." );
  } else {
    while( file_counts >> count >> Nc0 >> Nc1 ) {
      c_stars[count] = Nc1;
    }
    file_counts.close();
  }

  // The P(new_word) according to GoodTuring-3.pdf
  // We need the filename.cnt for this, because we really need to
  // discount the rest if we assign probability to the unseen words.
  //
  // We need to esitmate the total number of unseen words. Same as
  // vocab, i.e assume we saw half? Ratio of N_1 to total_count?
  //
  // We need to load .cnt file as well...
  //
  double p0 = 0.00001; // Arbitrary low probability for unknown words.
  if ( total_count > 0 ) { // Better estimate if we have a lexicon
    p0 = (double)N_1 / ((double)total_count * total_count);
  }
  //l.log( "P(new_particular) = " + to_str(p0) );

  for ( fi = filenames.begin(); fi != filenames.end(); fi++ ) {
    std::string a_file = *fi;
    std::string output_filename  = a_file + id + ".sc";
    std::string outlog_filename  = a_file + id + ".lg"; //LOG
    
    l.log( "Processing: "+a_file );
    l.log( "OUTPUT:     "+output_filename );
    l.log( "OUTPUT:     "+outlog_filename );
    
    if ( file_exists(l,c,output_filename) ) {
      l.log( "OUTPUT files exist, not overwriting." );
      c.add_kv( "sc_file", output_filename );
      l.log( "SET sc_file to "+output_filename );
      continue;
    }

    l.inc_prefix();

    std::ifstream file_in( a_file.c_str() );
    if ( ! file_in ) {
      l.log( "ERROR: cannot load inputfile." );
      return -1;
    }
    std::ofstream file_out( output_filename.c_str(), std::ios::out );
    if ( ! file_out ) {
      l.log( "ERROR: cannot write output file." );
      return -1;
    }
    std::ofstream log_out( outlog_filename.c_str(), std::ios::out );
    if ( ! log_out ) {
      l.log( "ERROR: cannot write log file." );
      return -1;
    }

    std::vector<std::string>::iterator vi;
    std::ostream_iterator<std::string> output( file_out, " " );

    std::string a_line;
	std::string classify_line;
    std::vector<std::string> results;
    std::vector<std::string> targets;
    std::vector<std::string>::iterator ri;
    const Timbl::ValueDistribution *vd;
    const Timbl::TargetValue *tv;
    std::vector<std::string> words;
    int correct = 0;
    int wrong   = 0;
    int correct_unknown = 0;
    int correct_distr = 0;

    // Recognise <s> or similar, reset pplx calculations.
    // Output results on </s> or similar.
    // Or a divisor which is not processed?
    //
    double sentence_prob      = 0.0;
    double sum_logprob        = 0.0;
    int    sentence_wordcount = 0;

    std::vector<std::string> cls;
    
    std::map<std::string, Expert*>::iterator tsi; //triggers
    Expert *e = NULL; // Expert used
    int pos;
    std::string trigger;
    
    while( std::getline( file_in, classify_line )) {
      
      cls.clear();
      words.clear();
      classify_line = trim( classify_line );
      
      if ( mode == 1 ) { // plain text, must be windowed
	window( classify_line, classify_line, lc, rc, (bool)false, 0, cls );
      } else {
	cls.push_back( classify_line );
      }
      
      for ( size_t i = 0; i < cls.size(); i++ ) {
	
	words.clear();
	a_line = cls.at(i);
	a_line = trim( a_line );
	
	Tokenize( a_line, words, ' ' );
	
	if ( hapax > 0 ) {
	  int c = hapax_vector( words, hpxfreqs, hapax );
	  vector_to_string(words, a_line);
	  a_line = trim( a_line );
	  words.clear();
	  Tokenize( a_line, words, ' ' );
	}
	
	std::string target = words.at( words.size()-1 );
	
	++sentence_wordcount;
	
	// Is the target in the lexicon? We could calculate a smoothed
	// value here if we load the .cnt file too...
	//
	std::map<std::string,int>::iterator wfi = wfreqs.find( target );
	bool   target_unknown = false;
	bool   correct_answer = false;
	double target_lexfreq = 0.0;// should be double because smoothing
	double target_lexprob = 0.0;
	if ( wfi == wfreqs.end() ) {
	  target_unknown = true;
	} else {
	  target_lexfreq =  (int)(*wfi).second; // Take lexfreq, unless we smooth
	  std::map<int,double>::iterator cfi = c_stars.find( target_lexfreq );
	  if ( cfi != c_stars.end() ) { // We have a smoothed value, use it
	    target_lexfreq = (double)(*cfi).second;
	    //l.log( "smoothed_lexfreq = " + to_str(target_lexfreq) );
	  }
	  target_lexprob = (double)target_lexfreq / (double)total_count;
	}
	
	// Do we Timbl, check triggers.
	pos     = words.size()-1-offset;
	pos     = (pos < 0) ? 0 : pos;
	trigger = words[pos];
	//l.log( "trigger: "+trigger);
	
	tsi = triggers.find( trigger );
	if ( tsi != triggers.end() ) {
	  e = (*tsi).second;
	} else { // the default classifier.
	  e = NULL;
	}
	
	if ( e == NULL ) {
	  // We print a dummy line, same format as normal experiments.
	  file_out << a_line << " (" << target << ") 0 0 1 1 [ ]\n";
	  continue;
	}
	
	// What does Timbl think?
	// Do we change this answer to what is in the distr. (if it is?)
	//
	tv = e->My_Experiment->Classify( a_line, vd );
	e->call();
	called[trigger] += 1; //we know it exists
	if ( ! tv ) {
	  l.log( "ERROR: Timbl returned a classification error, aborting." );
	  l.log( "ERROR: "+a_line );
	  break;
	}
	std::string answer = tv->Name();
	//l.log( "Answer: '" + answer + "' / '" + target + "'" );
	
	// Check match-depth, if too undeep, we are probably
	// unsure.
	//
	size_t md  = 0; //My_Experiment->matchDepth();
	bool   mal = 0; //My_Experiment->matchedAtLeaf();
	
	if ( target == answer ) {
	  ++correct;
	  correct_answer = true;
	} else {
	  ++wrong;
	}
	
	// Loop over distribution returned by Timbl.
	//
	Timbl::ValueDistribution::dist_iterator it = vd->begin();
	int cnt = 0;
	int distr_count = 0;
	int target_freq = 0;
	int answer_freq = 0;
	double prob            = 0.0;
	double target_distprob = 0.0;
	double answer_prob     = 0.0;
	double entropy         = 0.0;
	bool in_distr          = false;
	cnt = vd->size();
	distr_count = vd->totalSize(); //sum of freqs, for min_dsum
	double answer_f = 0;
	
	// Check if target word is in the distribution.
	//
	while ( it != vd->end() ) {
	  //const Timbl::TargetValue *tv = it->second->Value();
	  
	  std::string tvs  = it->second->Value()->Name();
	  double      wght = it->second->Weight();
	  
	  // Prob. of this item in distribution.
	  //
	  prob     = (double)wght / (double)distr_count;
	  entropy -= ( prob * log2(prob) );
	  
	  if ( tvs == target ) { // The correct answer was in the distribution!
	    target_freq = wght;
	    in_distr = true;
	    if ( correct_answer == false ) {
	      ++correct_distr;
	      --wrong; // direct answer wrong, but right in distr. compensate count
	    }
	  }
	  if ( tvs == answer ) { // Get the frequency to be able to calculate confidence
	    answer_f = wght;
	  }
	  
	  ++it;
	}
	target_distprob = (double)target_freq / (double)distr_count;
	
	// Confidence, after Skype call 20131101
	// frequency of top-1, the answer / sum(frequencies distribution)
	double the_confidence = -1; // -1 as shortcut to infinity
	if ( distr_count > 0 ) { // then there are more, with frequency, so we don't divide by 0
	  the_confidence = answer_f / distr_count;
	}
	
	// If correct: if target in distr, we take that prob, else
	// the lexical prob.
	// Unknown words?
	//
	double logprob = 0.0;
	if ( target_freq > 0 ) { // Right answer was in distr.
	  logprob = log2( target_distprob );
	} else {
	  if ( ! target_unknown ) { // Wrong, we take lex prob if known target
	    logprob = log2( target_lexprob ); // SMOOTHED here, see above
	  } else {
	    //
	    // What to do here? We have an 'unknown' target, i.e. not in the
	    // lexicon.
	    //
	    logprob = log2( p0 /*0.0001*/ ); // Foei!
	  }
	}
	sum_logprob += logprob;
	
	//l.log( "Target: "+target+" target_lexfreq: "+to_str(target_lexfreq) );

	// Simplified for cmcorrect, with only md and confidence.
	//
	std::vector<distr_elem*> distr_vec;
	bool fail = true;
	log_out << a_line << std::endl;
	if ( md >= (size_t)min_md ) {
	  log_out << "md [" << md << "] >= min_md [" << min_md << "]" << std::endl;
	  if ( (the_confidence >= confidence) || ( the_confidence < 0 ) ) {
	    log_out << "the_confidence [" << the_confidence << "] >= confidence [" << confidence << "] or the_confidence < 0 " << std::endl;
	    fail = false;
	    double sum_f = copy_dist( vd, distr_vec); // to output [ ... ]
	    // this is our classification
	    double word_lp = pow( 2, -logprob );
	    file_out << a_line << " (" << answer << ") "
		     << logprob << ' ' /*<< info << ' '*/ << entropy << ' ';
	    file_out << word_lp << ' ';
	    file_out << cnt << " [ ";
	    if ( topn > 0 ) {
	      int cntr = topn;
	      //sort( distr_vec.begin(), distr_vec.end(), distr_elem_cmprev_ptr() );
	      std::vector<distr_elem*>::const_iterator fi = distr_vec.begin();
	      while ( (fi != distr_vec.end()) && (cntr-- != 0) ) {
		file_out << (*fi)->name << ' ' << (double)((*fi)->freq) << ' ';
		fi++;
	      }
	    }
	    distr_vec.clear();
	    file_out << "]";
	    file_out << std::endl;
	  } // confidence
	}// md
	if ( fail == true ) {
	  log_out << "FAIL the_confidence [" << the_confidence << "] < confidence [" << confidence << "]" << std::endl;
	  // no classification, leave the text as it is
	  file_out << a_line << " (" << target << ") 0 0 1 1 [ ]\n";
	  distr_vec.clear();
	}
	log_out << std::endl;
	
	// End of sentence (sort of)
	//
	if ( target == "</s>" ) {
	  l.log( " sum_logprob = " + to_str( sum_logprob) );
	  l.log( " sentence_wordcount = " + to_str( sentence_wordcount ) );
	  double foo  = sum_logprob / (double)sentence_wordcount;
	  double pplx = pow( 2, -foo );
	  l.log( " pplx = " + to_str( pplx ) );
	  sum_logprob = 0.0;
	  sentence_wordcount = 0;
	  l.log( "--" );
	}
	
      } // i.cls loop
      
    } // while getline()
    
    if ( sentence_wordcount > 0 ) { // Overgebleven zooi (of alles).
      l.log( "sum_logprob = " + to_str( sum_logprob) );
      l.log( "sentence_wordcount = " + to_str( sentence_wordcount ) );
      double foo  = sum_logprob / (double)sentence_wordcount;
      double pplx = pow( 2, -foo );
      l.log( "pplx = " + to_str( pplx ) );
    }

    log_out.close();
    file_out.close();
    file_in.close();
    
    l.log( "Correct:       " + to_str(correct) );
    l.log( "Correct Distr: " + to_str(correct_distr) );
    int correct_total = correct_distr+correct;
    l.log( "Correct Total: " + to_str(correct_total) );
    l.log( "Wrong:         " + to_str(wrong) );
    if ( sentence_wordcount > 0 ) {
      l.log( "Cor.tot/total: " + to_str(correct_total / (double)sentence_wordcount) );
      l.log( "Correct/total: " + to_str(correct / (double)sentence_wordcount) );
    }
    
    // print call statistics for triggers.
    //
    std::map<std::string,int>::const_iterator ci = called.begin();
    while ( ci != called.end() ) {
      l.log( ci->first+": "+to_str(ci->second) );
      ci++;
    }
    
    c.add_kv( "sc_file", output_filename );
    l.log( "SET sc_file to "+output_filename );
    
    l.dec_prefix();
    
  }
  
  return 0;
}
#else
int cmcorrect( Logfile& l, Config& c ) {
  l.log( "No TIMBL support." );
  return -1;
}
#endif

// 2015-03-28

// Only leave in distr_vec what is in vd and a_set.
// Used in sml.
double filter_dist( const Timbl::ValueDistribution *vd, std::vector<std::string>& a_set, std::vector<distr_elem*>& distr_vec) {

  int    cnt         = 0;
  double distr_count = 0;
  
  Timbl::ValueDistribution::dist_iterator it = vd->begin();
  std::vector<std::string>::iterator si;
  
  while ( it != vd->end() ) { // loop over distribution
    std::string tvs  = it->second->Value()->Name(); // element in distribution
    double      wght = it->second->Weight(); // frequency of distr. element

    for ( si = a_set.begin(); si != a_set.end(); si++ ) { //should be outer
      if ( (*si) == tvs ) {
	distr_elem *d = new distr_elem(tvs, wght, 0); //last index?
	distr_vec.push_back( d );
	++cnt;
	distr_count += wght;
      }
    } // si
    if ( distr_vec.size() == a_set.size() ) {
      break;
    }
    ++it;
  }
  sort( distr_vec.begin(), distr_vec.end(), distr_elem_cmprev_ptr() );
  return distr_count; //cnt is size()
}

// for cmcorrect
double copy_dist( const Timbl::ValueDistribution *vd, std::vector<distr_elem*>& distr_vec) {
  int    cnt         = 0;
  double distr_count = 0;
  
  Timbl::ValueDistribution::dist_iterator it = vd->begin();
  std::vector<std::string>::iterator si;
  
  while ( it != vd->end() ) { // loop over distribution
    std::string tvs  = it->second->Value()->Name(); // element in distribution
    double      wght = it->second->Weight(); // frequency of distr. element

    distr_elem *d = new distr_elem(tvs, wght, 0); //last index?
    distr_vec.push_back( d );
    ++cnt;
    distr_count += wght;

    ++it;
  }
  sort( distr_vec.begin(), distr_vec.end(), distr_elem_cmprev_ptr() );
  return distr_count; //cnt is size()
}

// Confidence only. Add modes for examining distribution?
//
#ifdef TIMBL
int sml( Logfile& l, Config& c ) {
  l.log( "sml" );
  const std::string& filename         = c.get_value( "filename" );
  std::string        dirname          = c.get_value( "dir", "" );
  std::string        dirmatch         = c.get_value( "dirmatch", ".*" );
  const std::string& ibasefile        = c.get_value( "ibasefile" );
  const std::string& lexicon_filename = c.get_value( "lexicon" );
  const std::string& counts_filename  = c.get_value( "counts" );
  const std::string& trigger_filename = c.get_value( "triggerfile", "" );
  const std::string& timbl            = c.get_value( "timbl" );
  const int          hapax            = my_stoi( c.get_value( "hpx", "0" ));
  const int          mode             = my_stoi( c.get_value( "mode", "0" )); // mode:0 is windowed, mode:1 is plain text
  const int          lc               = my_stoi( c.get_value( "lc", "2" ));
  const int          rc               = my_stoi( c.get_value( "rc", "0" ));
  std::string        id               = c.get_value( "id", to_str(getpid()) );
  //std::string        output_filename  = filename + id + ".sc";
  std::string        pre_s            = c.get_value( "pre_s", "<s>" );
  std::string        suf_s            = c.get_value( "suf_s", "</s>" );
  // Ratio between top-1 frequency and sum of rest of distribution frequencies
  double             confidence      = my_stod( c.get_value( "confidence", "0" ));
  int                topn            = my_stoi( c.get_value( "topn", "0" ) );
  
  Timbl::TimblAPI   *My_Experiment;
  std::string        distrib;
  std::vector<std::string> distribution;
  std::string        result;
  double             distance;

#if defined(HAVE_TR1_UNORDERED_SET)
  std::tr1::unordered_set<std::string> triggers;
#else
  std::unordered_set<std::string> triggers;
#endif

  // No slash at end of dirname.
  //
  if ( (dirname != "") && (dirname.substr(dirname.length()-1, 1) == "/") ) {
    dirname = dirname.substr(0, dirname.length()-1);
  }

  l.inc_prefix();
  if ( dirname != "" ) {
    l.log( "dir:             "+dirname );
    l.log( "dirmatch:        "+dirmatch );
  }
  //l.log( "filename:   "+filename );
  l.log( "ibasefile:  "+ibasefile );
  l.log( "lexicon:    "+lexicon_filename );
  l.log( "counts:     "+counts_filename );
  l.log( "triggerfile:"+trigger_filename );
  l.log( "timbl:      "+timbl );
  l.log( "id:         "+id );
  l.log( "hapax:      "+to_str(hapax) );
  l.log( "mode:       "+to_str(mode) );
  l.log( "topn:       "+to_str(topn) );
  l.log( "lc:         "+to_str(lc) ); // left context size for windowing
  l.log( "rc:         "+to_str(rc) ); // right context size for windowing
  l.log( "confidence  "+to_str(confidence) );
  
  //l.log( "OUTPUT:     "+output_filename );
  l.dec_prefix();

  // Timbl
  if ( ! file_exists(l, c, ibasefile) ) {
    l.log( "ERROR: cannot read "+ibasefile+"." );
    return -1;
  }

  // One file, as before, or the whole globbed dir. Check if
  // they exists.
  //
  std::vector<std::string> filenames;
  std::vector<std::string>::iterator fi;
  if ( dirname == "" ) {
    filenames.push_back( filename );
  } else {
    get_dir( dirname, filenames, dirmatch );
  }
  l.log( "Processing "+to_str(filenames.size())+" files." );
  size_t numfiles = filenames.size();
  if ( numfiles == 0 ) {
    l.log( "No files found. Skipping." );
    return 0;
  }

  if ( contains_id(filenames[0], id) == true ) {
    id = "";
  } else {
    id = "_"+id;
  }

  for ( fi = filenames.begin(); fi != filenames.end(); fi++ ) {
    std::string a_file = *fi;
    std::string output_filename  = a_file + id + ".sc";

    if (file_exists(l,c,output_filename)) {
      //l.log( "Output for "+a_file+" exists, removing from list." );
      --numfiles;
    }
  }
  if ( numfiles == 0 ) {
    l.log( "All output files already exists, skipping." );
    return 0;
  }

  // NB reads commented lines
  int trigger_count = 0;
  if ( trigger_filename != "" ) {
    std::ifstream file_triggers( trigger_filename.c_str() );
    if ( ! file_triggers ) {
      l.log( "NOTICE: cannot load specified triggerfile." );
      //return -1;
    } else {
      l.log( "Reading triggers." );
      std::string a_word;
      while ( file_triggers >> a_word ) { // should skip #
	triggers.insert(a_word);
	++trigger_count;
      }
      
      file_triggers.close();
      l.log( "Read triggers (total_count="+to_str(trigger_count)+")." );
    }
  }

  // PJB we need as sets, with other member to be able to do
  // selective ML.
  // vector of sets, set of sets, vector of vectors...
  // triggerword -> [ indexes in array with other set members ]
  // 0) read in each line as vector, Tokenize( a_line, words, ' ' )
  // 1) store these
  //
  // Read as sets
  std::vector<std::string> words;
  // their -> [their, there, they're]
  std::map<std::string,std::vector<std::string>> c_sets;
  if ( trigger_filename != "" ) {
    std::ifstream file_triggers( trigger_filename.c_str() );
    if ( ! file_triggers ) {
      l.log( "NOTICE: cannot load sets." );
      //return -1;
    } else {
      l.log( "Reading sets." );
      std::string a_line;
      int set_count = 0;
      std::vector<std::string>::iterator wi;
      while( std::getline( file_triggers, a_line )) {
	if ( a_line[0] != '#' ) {
	  std::string a_word;
	  words.clear();
	  ++set_count;
	  Tokenize( a_line, words );
	  std::vector<std::string> c_set;
	  for ( wi = words.begin(); wi != words.end(); wi++ ) {	  
	    c_set.push_back(*wi);
	    //l.log( (*wi)+" "+a_line);
	  }
	  for ( wi = words.begin(); wi != words.end(); wi++ ) {	  
	    c_sets[*wi] = c_set;
	  }
	}
      }
      file_triggers.close();
      l.log( "Read sets ("+to_str(set_count)+")." );
    }
  }
  
  try {
    My_Experiment = new Timbl::TimblAPI( timbl );
    if ( ! My_Experiment->Valid() ) {
      l.log( "Timbl Experiment is not valid." );
      return 1;
    }
    (void)My_Experiment->GetInstanceBase( ibasefile );
    if ( ! My_Experiment->Valid() ) {
      l.log( "Timbl Experiment is not valid." );
      return 1;
    }
  } catch (...) {
    l.log( "Cannot create Timbl Experiment..." );
    return 1;
  }
  l.log( "Instance base loaded." );

  // Load lexicon. NB: hapaxed lexicon is different? Or add HAPAX entry?
  //
  int wfreq;
  unsigned long total_count = 0;
  unsigned long N_1 = 0; // Count for p0 estimate.
  unsigned long hpx_entries = 0;
  std::map<std::string,int> wfreqs; // whole lexicon
  std::map<std::string,int> hpxfreqs; // hapaxed list
  std::ifstream file_lexicon( lexicon_filename.c_str() );
  if ( ! file_lexicon ) {
    l.log( "NOTICE: cannot load lexicon file." );
    //return -1;
  } else {
    // Read the lexicon with word frequencies.
    // We need a hash with frequence - countOfFrequency, ffreqs.
    //
    l.log( "Reading lexicon." );
    std::string a_word;
    while ( file_lexicon >> a_word >> wfreq ) {
      wfreqs[a_word] = wfreq;
      total_count += wfreq;
      if ( wfreq == 1 ) {
	++N_1;
      }
      if ( wfreq > hapax ) {
	hpxfreqs[a_word] = wfreq;
	++hpx_entries;
      }
    }
    file_lexicon.close();
    l.log( "Read lexicon (total_count="+to_str(total_count)+")." );
  }

  // If we want smoothed counts, we need this file...
  // Make mapping <int, double> from c to c* ?
  //
  std::map<int,double> c_stars;
  int Nc0;
  double Nc1; // this is c*
  int count;
  std::ifstream file_counts( counts_filename.c_str() );
  if ( ! file_counts ) {
    l.log( "NOTICE: cannot read counts file, no smoothing will be applied." );
  } else {
    while( file_counts >> count >> Nc0 >> Nc1 ) {
      c_stars[count] = Nc1;
    }
    file_counts.close();
  }
  
  // The P(new_word) according to GoodTuring-3.pdf
  // We need the filename.cnt for this, because we really need to
  // discount the rest if we assign probability to the unseen words.
  //
  // We need to esitmate the total number of unseen words. Same as
  // vocab, i.e assume we saw half? Ratio of N_1 to total_count?
  //
  // We need to load .cnt file as well...
  //
  double p0 = 0.00001; // Arbitrary low probability for unknown words.
  if ( total_count > 0 ) { // Better estimate if we have a lexicon
    p0 = (double)N_1 / ((double)total_count * total_count);
  }
  //l.log( "P(new_particular) = " + to_str(p0) );

  for ( fi = filenames.begin(); fi != filenames.end(); fi++ ) {
    std::string a_file = *fi;
    std::string output_filename  = a_file + id + ".sc";
    std::string outlog_filename  = a_file + id + ".lg"; //LOG

    l.log( "Processing: "+a_file );
    l.log( "OUTPUT:     "+output_filename );
    l.log( "OUTPUT:     "+outlog_filename );

    if ( file_exists(l,c,output_filename) ) {
      l.log( "OUTPUT files exist, not overwriting." );
      c.add_kv( "sc_file", output_filename );
      l.log( "SET sc_file to "+output_filename );
      continue;
    }

    l.inc_prefix();

    std::ifstream file_in( a_file.c_str() );
    if ( ! file_in ) {
      l.log( "ERROR: cannot load inputfile." );
      return -1;
    }
    std::ofstream file_out( output_filename.c_str(), std::ios::out );
    if ( ! file_out ) {
      l.log( "ERROR: cannot write output file." );
      return -1;
    }
    std::ofstream log_out( outlog_filename.c_str(), std::ios::out ); 
   if ( ! log_out ) {
      l.log( "ERROR: cannot write log file." );
      return -1;
    }

    std::vector<std::string>::iterator vi;
    std::ostream_iterator<std::string> output( file_out, " " );

    std::string a_line;
    std::string classify_line;
    std::vector<std::string> results;
    std::vector<std::string> targets;
    std::vector<std::string>::iterator ri;
    const Timbl::ValueDistribution *vd;
    const Timbl::TargetValue *tv;
    int correct = 0;
    int wrong   = 0;
    int correct_unknown = 0;
    int correct_distr = 0;

    // Recognise <s> or similar, reset pplx calculations.
    // Output results on </s> or similar.
    // Or a divisor which is not processed?
    //
    double sentence_prob      = 0.0;
    double sum_logprob        = 0.0;
    int    sentence_wordcount = 0;

    std::vector<std::string> cls;

    while( std::getline( file_in, classify_line )) {

      cls.clear();
      words.clear();
      classify_line = trim( classify_line );
      
      if ( mode == 1 ) { // plain text, must be windowed
	window( classify_line, classify_line, lc, rc, (bool)false, 0, cls );
      } else {
	cls.push_back( classify_line );
      }
      
      for ( size_t i = 0; i < cls.size(); i++ ) {
	
	words.clear();
	a_line = cls.at(i);
	a_line = trim( a_line );
	
	Tokenize( a_line, words, ' ' );
	
	if ( hapax > 0 ) {
	  int c = hapax_vector( words, hpxfreqs, hapax );
	  vector_to_string(words, a_line);
	  a_line = trim( a_line );
	  words.clear();
	  Tokenize( a_line, words, ' ' );
	}
	
	// The answer we are after, last feature value in instance.
	std::string target = words.at( words.size()-1 );
	
	++sentence_wordcount;
	
	// Is the target in the lexicon? We could calculate a smoothed
	// value here if we load the .cnt file too...
	//
	std::map<std::string,int>::iterator wfi = wfreqs.find( target );
	bool   target_unknown = false;
	bool   correct_answer = false;
	double target_lexfreq = 0.0;// should be double because smoothing
	double target_lexprob = 0.0;
	if ( wfi == wfreqs.end() ) {
	  target_unknown = true;
	} else {
	  target_lexfreq =  (int)(*wfi).second; // Take lexfreq, unless we smooth
	  std::map<int,double>::iterator cfi = c_stars.find( target_lexfreq );
	  if ( cfi != c_stars.end() ) { // We have a smoothed value, use it
	    target_lexfreq = (double)(*cfi).second;
	    //l.log( "smoothed_lexfreq = " + to_str(target_lexfreq) );
	  }
	  target_lexprob = (double)target_lexfreq / (double)total_count;
	}
	
	// Do nothing if not a trigger, write default line.
	//
	if ( trigger_count > 0 ) {
	  if ( triggers.find(target) == triggers.end() ) {
	    file_out << a_line << " (" << target << ") "
		     << 0 << ' ' /*<< info << ' '*/ << 0 << ' ';
	    file_out << 0 << ' ';
	    file_out << 0 << " [ ";
	    file_out << "]";
	    file_out << std::endl;
	    continue;
	  }
	}

	// What does Timbl think?
	// Do we change this answer to what is in the distr. (if it is?)
	//
	tv = My_Experiment->Classify( a_line, vd );
	if ( ! tv ) {
	  l.log( "ERROR: Timbl returned a classification error, aborting." );
	  break;
	}
	std::string answer = tv->Name();
	//l.log( "Answer: '" + answer + "' / '" + target + "'" );
	
	// Check match-depth, if too undeep, we are probably
	// unsure.
	//
	size_t md  = My_Experiment->matchDepth();
	bool   mal = My_Experiment->matchedAtLeaf();
	
	if ( target == answer ) {
	  ++correct;
	  correct_answer = true;
	} else {
	  ++wrong;
	}
	
	// Loop over distribution returned by Timbl.
	//
	Timbl::ValueDistribution::dist_iterator it = vd->begin();
	int cnt = 0;
	int distr_count = 0;
	int target_freq = 0;
	int answer_freq = 0;
	double prob            = 0.0;
	double target_distprob = 0.0;
	double answer_prob     = 0.0;
	double entropy         = 0.0;
	bool in_distr          = false;
	cnt = vd->size();
	distr_count = vd->totalSize();
	double answer_f = 0;
	
	// Check if target word is in the distribution.
	//
	while ( it != vd->end() ) {
	  //const Timbl::TargetValue *tv = it->second->Value();
	  
	  std::string tvs  = it->second->Value()->Name();
	  double      wght = it->second->Weight();
	  
	  // Prob. of this item in distribution.
	  //
	  prob     = (double)wght / (double)distr_count;
	  entropy -= ( prob * log2(prob) );
	  
	  if ( tvs == target ) { // The correct answer was in the distribution!
	    target_freq = wght;
	    in_distr = true;
	    if ( correct_answer == false ) {
	      ++correct_distr;
	      --wrong; // direct answer wrong, but right in distr. compensate count
	    }
	  }
	  if ( tvs == answer ) { // Get the frequency to be able to calculate confidence
	    answer_f = wght;
	  }
	  
	  ++it;
	}
	target_distprob = (double)target_freq / (double)distr_count;
	
	double the_confidence = -1; // -1 as shortcut to infinity
	if ( distr_count > 0 ) { 
	  the_confidence = answer_f / distr_count;
	}
	
	// If correct: if target in distr, we take that prob, else
	// the lexical prob.
	// Unknown words?
	//
	double logprob = 0.0;
	if ( target_freq > 0 ) { // Right answer was in distr.
	  logprob = log2( target_distprob );
	} else {
	  if ( ! target_unknown ) { // Wrong, we take lex prob if known target
	    logprob = log2( target_lexprob ); // SMOOTHED here, see above
	  } else {
	    //
	    // What to do here? We have an 'unknown' target, i.e. not in the
	    // lexicon.
	    //
	    logprob = log2( p0 /*0.0001*/ ); // Foei!
	  }
	}
	sum_logprob += logprob;
	
	std::vector<distr_elem*> distr_vec;
	bool fail = true;
	log_out << a_line << std::endl;

	// Filter distro, recalc confidence
	std::vector<std::string> c_set = c_sets[target]; //check error(?)
	std::vector<std::string>::iterator si;
	std::string top_c;
	double top_f = -1;
	double sum_f = 0;
	size_t cc = 0;
	std::vector<distr_elem*> filtered_distr_vec;
	std::vector<distr_elem*>::const_iterator fi;
	
	sum_f = filter_dist( vd, c_set, filtered_distr_vec);
	cc = filtered_distr_vec.size();
	
	/*
[pberck@margretetorp cmcorrect]$ ~/uvt/wopr/src/wopr -l -r sml -p ibasefile:utexas.10e6.dt3.100000.l2r2_-a1+D.ibase,timbl:"-a1 +D",filename:utexas.10e6.dt3.t1e5d.cf350.l2r2,triggerfile:goldingroth3a.txt,lc:2,rc:2,confidence:0,id:SML4F
	*/
	the_confidence = -1;
	if ( cc > 0 ) { // found 1 or more elements from set
	  if ( sum_f > 0 ) { // it should be...
	    top_c = filtered_distr_vec[0]->name;
	    top_f = filtered_distr_vec[0]->freq;
	    the_confidence = top_f / sum_f;
	  }
	  if ( (the_confidence >= confidence) || ( the_confidence < 0 ) ) {
	    log_out << "found " << cc << " in distribution of " << vd->size() << std::endl;
	    log_out << "the_confidence [" << the_confidence << "] >= confidence [" << confidence << "] or the_confidence < 0 " << std::endl;
	    fail = false;
	    
	    Timbl::ValueDistribution::dist_iterator it = vd->begin();
	    std::map<std::string,int>::iterator tvsfi;
	    std::map<std::string,int>::iterator wfi;
	    double factor = 0.0;
	    
	    // This is our answer, the stats are on whole distr. 
	    answer = top_c;
	    
	    double word_lp = pow( 2, -logprob );
	    file_out << a_line << " (" << answer << ") "
		     << logprob << ' ' /*<< info << ' '*/ << entropy << ' ';
	    file_out << word_lp << ' ';
	    file_out << cnt << " [ ";
	    if ( topn > 0 ) {
	      int cntr = topn;
	      //sort( filtered_distr_vec.begin(), filtered_distr_vec.end(), distr_elem_cmprev_ptr() ); //NB: cmprev (versus cmp) //is returned sorted
	      //std::vector<distr_elem*>::const_iterator fi = filtered_distr_vec.begin();
	      fi = filtered_distr_vec.begin();
	      while ( (fi != filtered_distr_vec.end()) && (cntr-- != 0) ) {
		file_out << (*fi)->name << ' ' << (double)((*fi)->freq) << ' ';
		delete *fi;
		fi++;
	      }
	    }
	    distr_vec.clear();
	    filtered_distr_vec.clear();
	    file_out << "]";
	    file_out << std::endl;
	  } // confidence
	} else { // cc > 0
	  log_out << "FAIL No elements found in distribution." << std::endl;
	}
	if ( fail == true ) {
	  log_out << "FAIL the_confidence [" << the_confidence << "] < confidence [" << confidence << "]" << std::endl;
	  // no classification, leave the text as it is
	  file_out << a_line << " (" << target << ") 0 0 1 1 [ ]\n";
	}
	log_out << std::endl;
	
	// End of sentence (sort of)
	//
	if ( target == "</s>" ) {
	  l.log( " sum_logprob = " + to_str( sum_logprob) );
	  l.log( " sentence_wordcount = " + to_str( sentence_wordcount ) );
	  double foo  = sum_logprob / (double)sentence_wordcount;
	  double pplx = pow( 2, -foo );
	  l.log( " pplx = " + to_str( pplx ) );
	  sum_logprob = 0.0;
	  sentence_wordcount = 0;
	  l.log( "--" );
	}
	
      } // i.cls loop

    } // while getline()

    if ( sentence_wordcount > 0 ) { // Overgebleven zooi (of alles).
      l.log( "sum_logprob = " + to_str( sum_logprob) );
      l.log( "sentence_wordcount = " + to_str( sentence_wordcount ) );
      double foo  = sum_logprob / (double)sentence_wordcount;
      double pplx = pow( 2, -foo );
      l.log( "pplx = " + to_str( pplx ) );
    }

    log_out.close();
    file_out.close();
    file_in.close();

    l.log( "Correct:       " + to_str(correct) );
    l.log( "Correct Distr: " + to_str(correct_distr) );
    int correct_total = correct_distr+correct;
    l.log( "Correct Total: " + to_str(correct_total) );
    l.log( "Wrong:         " + to_str(wrong) );
    if ( sentence_wordcount > 0 ) {
      l.log( "Cor.tot/total: " + to_str(correct_total / (double)sentence_wordcount) );
      l.log( "Correct/total: " + to_str(correct / (double)sentence_wordcount) );
    }

    c.add_kv( "sc_file", output_filename );
    l.log( "SET sc_file to "+output_filename );

    l.dec_prefix();

  }

  return 0;
}
#else
int sml( Logfile& l, Config& c ) {
  l.log( "No TIMBL support." );
  return -1;
}
#endif
