// ---------------------------------------------------------------------------
// $Id$
// ---------------------------------------------------------------------------

/*****************************************************************************
 * Copyright 2007 - 2013 Peter Berck                                         *
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

#include "qlog.h"
#include "util.h"
#include "Config.h"
#include "runrunrun.h"
#include "tag.h"
#include "Classifier.h"
#include "Multi.h"
#include "server.h"
#include "levenshtein.h"
#include "elements.h"
#include "cache.h"

#ifdef TIMBL
# include "timbl/TimblAPI.h"
#endif

#ifdef TIMBLSERVER
#include "SocketBasics.h"
#endif

#define BACKLOG 5        // how many pending connections queue will hold
#define MAXDATASIZE 2048 // max number of bytes we can get at once 


#ifndef TRANSPLD2
const int transpld = 2;
#else
const int transpld = 1;
#endif

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
		     std::vector<distr_elem*>& distr_vec, int mld, double min_ratio, double target_lexfreq, bool cs) {

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
		( ld <= mld )
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
      // ratio, we store the ditribution element (possible correction).
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
  const std::string& timbl            = c.get_value( "timbl" );
  std::string        id               = c.get_value( "id", to_str(getpid()) );

  //std::string        output_filename  = filename + id + ".sc";
  std::string        pre_s            = c.get_value( "pre_s", "<s>" );
  std::string        suf_s            = c.get_value( "suf_s", "</s>" );
  // minimum word length (guess added if > mwl)
  int                mwl              = stoi( c.get_value( "mwl", "5" ) );
  // maximum levenshtein distance (guess added if <= mld)
  int                mld              = stoi( c.get_value( "mld", "1" ) );
  // max entropy (guess added if <= max_entropy)
  int                max_ent          = stoi( c.get_value( "max_ent", "5" ) );
  // maximum distributie (guess added if <= max_distr)
  int                max_distr        = stoi( c.get_value( "max_distr", "10" ));
  // ratio target_lexfreq:tvs_lexfreq
  double             min_ratio        = stod( c.get_value( "min_ratio", "0" ));
  // maximum target frequency (word under scrutiny is not in dist or (<=) very low freq)
  int                max_tf           = stoi( c.get_value( "max_tf", "1" ));
  int                skip             = 0;
  bool               cs               = stoi( c.get_value( "cs", "1" )) == 1; //case insensitive levenshtein cs:0

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
  l.log( "timbl:      "+timbl );
  l.log( "id:         "+id );
  l.log( "mwl:        "+to_str(mwl) );
  l.log( "mld:        "+to_str(mld) );
  l.log( "max_ent:    "+to_str(max_ent) );
  l.log( "max_distr:  "+to_str(max_distr) );
  l.log( "min_ratio:  "+to_str(min_ratio) );
  l.log( "max_tf:     "+to_str(max_tf) );
  l.log( "cs:         "+to_str(cs) );
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
  std::map<std::string,int> wfreqs; // whole lexicon
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

	// header
	//
#ifndef TRANSPLD2
	file_out << "# cs:"+to_str(cs)+" transpose:1" << std::endl;
#else
	file_out << "# cs:"+to_str(cs)+" transpose:2" << std::endl;
#endif

    std::vector<std::string>::iterator vi;
    std::ostream_iterator<std::string> output( file_out, " " );

    std::string a_line;
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

    while( std::getline( file_in, a_line )) {

      words.clear();
      a_line = trim( a_line );
      Tokenize( a_line, words, ' ' );
      if ( words.size() == 1 ) { // For Hermans data. TODO: better fix.
		words.clear();
		Tokenize( a_line, words, '\t' );
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
		
		++it;
      }
      target_distprob = (double)target_freq / (double)distr_count;
	  
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
      if ( (cnt <= max_distr) && (target.length() > mwl) && ((in_distr == false)||(target_freq<=max_tf)) && (entropy <= max_ent) ) {
		distr_spelcorr( vd, target, wfreqs, distr_vec, mld, min_ratio, target_lexfreq, cs);
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
  const int verbose             = stoi( c.get_value( "verbose", "0" ));
  const int keep                = stoi( c.get_value( "keep", "0" ));
  const std::string& lexicon_filename = c.get_value( "lexicon" );
  const long cachesize          = stoi( c.get_value( "cs", "100000" ));

  int                mwl              = stoi( c.get_value( "mwl", "5" ) );
  // maximum levenshtein distance (guess added if <= mld)
  int                mld              = stoi( c.get_value( "mld", "1" ) );
  // max entropy (guess added if <= max_entropy)
  int                max_ent          = stoi( c.get_value( "max_ent", "5" ) );
  // maximum distributie (guess added if <= max_distr)
  int                max_distr        = stoi( c.get_value( "max_distr", "10" ));
  // ratio target_lexfreq:tvs_lexfreq
  double             min_ratio        = stod( c.get_value( "min_ratio", "0" ));
  const std::string empty        = c.get_value( "empty", "__EMPTY__" );
  const int hapax                   = stoi( c.get_value( "hpx", "0" ));

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
	  for ( int i = 0; i < cls.size(); i++ ) {
	    
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
	      if ( (target.length() > mwl) && (in_distr == false) ) {
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
  const int verbose             = stoi( c.get_value( "verbose", "0" ));
  const int keep                = stoi( c.get_value( "keep", "0" ));
  const std::string& lexicon_filename = c.get_value( "lexicon" );

  int                mwl              = stoi( c.get_value( "mwl", "5" ) );
  // maximum levenshtein distance (guess added if <= mld)
  int                mld              = stoi( c.get_value( "mld", "1" ) );
  // max entropy (guess added if <= max_entropy)
  int                max_ent          = stoi( c.get_value( "max_ent", "5" ) );
  // maximum distributie (guess added if <= max_distr)
  int                max_distr        = stoi( c.get_value( "max_distr", "10" ));
  // ratio target_lexfreq:tvs_lexfreq
  double             min_ratio        = stod( c.get_value( "min_ratio", "0" ));
  const std::string empty        = c.get_value( "empty", "__EMPTY__" );

  const long cachesize            = stoi( c.get_value( "cs", "100000" ));

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
	for ( int i = 0; i < cls.size(); i++ ) {
	    
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
	    if ( (target.length() > mwl) && (in_distr == false) ) {
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
