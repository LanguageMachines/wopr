// ---------------------------------------------------------------------------
// $Id$
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
#include "Multi.h"
#include "wex.h"

// ---------------------------------------------------------------------------
//  Code.
// ---------------------------------------------------------------------------

#ifdef TIMBL
// wopr -r multi -p kvs:ibases.kvs,filename:zin.04,lexicon:austen.txt.ng2.lex
//
int multi( Logfile& l, Config& c ) {
  l.log( "multi" );
  const std::string& filename         = c.get_value( "filename" );
  const std::string& lexicon_filename = c.get_value( "lexicon" );
  const std::string& counts_filename  = c.get_value( "counts" );
  const std::string& kvs_filename     = c.get_value( "kvs" );
  const std::string& timbl            = c.get_value( "timbl" );
  // PJB: Should be a context string, also l2r2 &c.
  int                ws               = stoi( c.get_value( "ws", "3" ));
  bool               to_lower         = stoi( c.get_value( "lc", "0" )) == 1;
  std::string        output_filename  = filename + ".px" + to_str(ws);
  std::string        pre_s            = c.get_value( "pre_s", "<s>" );
  std::string        suf_s            = c.get_value( "suf_s", "</s>" );
  int                skip             = 0;
  Timbl::TimblAPI   *My_Experiment;
  std::string        distrib;
  std::vector<std::string> distribution;
  std::string        result;
  double             distance;

  l.inc_prefix();
  l.log( "filename:   "+filename );
  l.log( "lexicon:    "+lexicon_filename );
  l.log( "counts:     "+counts_filename );
  l.log( "kvs:        "+kvs_filename );
  l.log( "timbl:      "+timbl );
  l.log( "ws:         "+to_str(ws) );
  l.log( "lowercase:  "+to_str(to_lower) );
  l.log( "OUTPUT:     "+output_filename );
  l.dec_prefix();

  std::ifstream file_in( filename.c_str() );
  if ( ! file_in ) {
    l.log( "ERROR: cannot load inputfile." );
    return -1;
  }
  std::ofstream file_out( output_filename.c_str(), std::ios::out );
  if ( ! file_out ) {
    l.log( "ERROR: cannot write output file." );
    return -1;
  }

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

  // read kvs
  //
  std::map<int, Classifier*> ws_classifier; // size -> classifier.
  std::map<int, Classifier*>::iterator wsci;
  std::vector<Classifier*> cls;
  std::vector<Classifier*>::iterator cli;
  if ( kvs_filename != "" ) {
    l.log( "Reading classifiers." );
    std::ifstream file_kvs( kvs_filename.c_str() );
    if ( ! file_kvs ) {
      l.log( "ERROR: cannot load kvs file." );
      return -1;
    }
    read_classifiers_from_file( file_kvs, cls );
    l.log( to_str((int)cls.size()) );
    file_kvs.close();
    for ( cli = cls.begin(); cli != cls.end(); cli++ ) {
      l.log( (*cli)->id );
      (*cli)->init();
      int c_ws = (*cli)->get_ws();
      ws_classifier[c_ws] = *cli;
    }
    l.log( "Read classifiers." );
  }

  std::vector<std::string>::iterator vi;
  std::ostream_iterator<std::string> output( file_out, " " );

  std::string a_line;
  std::vector<std::string> results;
  std::vector<std::string> targets;
  std::vector<std::string>::iterator ri;
  const Timbl::ValueDistribution *vd;
  const Timbl::TargetValue *tv;
  std::vector<Multi*> multivec( 20 );
  std::vector<std::string> words;

  while( std::getline( file_in, a_line )) {

    if ( to_lower ) {
      std::transform(a_line.begin(),a_line.end(),a_line.begin(),tolower); 
    }

    // How long is the sentence? initialise Multi.h in advance,
    // one per word in the sentence?
    // Initialise correct target as well...
    //
    int multi_idx = 0;
    multivec.clear();
    words.clear();
    Tokenize( a_line, words, ' ' );
    int sentence_size = words.size();
    for ( int i = 0; i < sentence_size; i++ ) {
      multivec[i] = new Multi("");
      multivec[i]->set_target( words[i] );
    }

    // We loop over classifiers, vote which result we take.
    //
    for ( cli = cls.begin(); cli != cls.end(); cli++ ) {

      Classifier *classifier = *cli;
      l.log( "Classifier: " + (*cli)->id );
      int win_s = (*cli)->get_ws();
      //l.log( "win_s="+to_str(win_s) );

      Timbl::TimblAPI *timbl = classifier->get_exp();

      //      pattern target  lc    rc  backoff
      window( a_line, a_line, win_s, 0, false, results ); 

      // For each classifier, make data and run. We need to specify how to
      // make data. We have the ws parameter in the Classifier.
      //
      // If we get different size patterns (first 1, then 2, then 3, 3, 3) we
      // can choose he right classifier based on the size of the pattern.
      // So maybe we need yet-another-window function which does that
      // (essentially patterns without the _ markers).
      //
      // Add a function to the kvs file to make data?
      //
      // We need a data structure to gather all the results and probability
      // values... (classifier, classification, distr?, prob....)
      //
      multi_idx = 0;
      for ( ri = results.begin(); ri != results.end(); ri++ ) {
	std::string cl = *ri;
	file_out << cl << std::endl;

	tv = timbl->Classify( cl, vd );
	std::string answer = tv->Name();

	int cnt = vd->size();
	int distr_count = vd->totalSize();

	l.log( to_str(multi_idx) + ": " + cl + "/" + answer + " "+to_str(cnt) );

	multivec[multi_idx]->add_string( answer );
	multivec[multi_idx]->add_answer( classifier, answer );

	++multi_idx;
      } // results
      results.clear();
    } // classifiers

    for ( int i = 0; i < sentence_size; i++ ) {
      //l.log( words[i] + ": " + multivec[i]->get_answer() );
      l.log( multivec[i]->get_answers() );
    }
  }

  file_out.close();
  file_in.close();

  c.add_kv( "filename", output_filename );
  l.log( "SET filename to "+output_filename );
  return 0;
}
#else
int multi( Logfile& l, Config& c ) {
  l.log( "Timbl support not built in." );  
  return -1;
}
#endif

// Fill a keyword value hash
// What we want:
//
// classifier:one
// one:ibasefile:something.ws3.ibase
// one:ws:3
// one:foo:bar
// classifier:two
// ...
//
int read_kv_from_file( std::ifstream& file,
		       std::map<std::string, std::string>& res )  {
  std::string a_line;
  while( std::getline( file, a_line )) {
    if ( a_line.length() == 0 ) {
      continue;
    }
    int pos = a_line.find( ':', 0 );
    if ( pos != std::string::npos ) {
      std::string lhs = trim(a_line.substr( 0, pos ));
      std::string rhs = trim(a_line.substr( pos+1 ));
      res[lhs] = rhs;
    }
  }
  
  return 0;
}

int read_classifiers_from_file( std::ifstream& file,
				std::vector<Classifier*>& cl )  {
  std::string a_line;
  Classifier* c = NULL;
  while( std::getline( file, a_line )) {
    if ( a_line.length() == 0 ) {
      continue;
    }
    int pos = a_line.find( ':', 0 );
    if ( pos != std::string::npos ) {
      std::string lhs = trim(a_line.substr( 0, pos ));
      std::string rhs = trim(a_line.substr( pos+1 ));

      // Create a new one. If c != NULL, we store it in the cl vector.
      //
      if ( lhs == "classifier" ) {
	if ( c != NULL ) {
	  //store
	  cl.push_back( c );
	  c = NULL;
	}
	c = new Classifier( rhs );
      } else if ( lhs == "ibasefile" ) {
	// store this ibasefile
	if ( c != NULL ) {
	  c->set_ibasefile( rhs );
	}
      } else if ( lhs == "ws" ) {
	// store this window size
	if ( c != NULL) {
	    c->set_ws( stoi(rhs) );
	}
      } else if ( lhs == "timbl" ) {
	// e.g: "-a1 +vdb +D +G"
	if ( c != NULL) {
	    c->set_timbl( rhs );
	}
      }
    }
  }
  if ( c != NULL ) {
    cl.push_back( c );
  }
  
  return 0;
}

// After discussion with Antal 09/12/09, mixing of distributions.
// 
#ifdef TIMBL
struct distr_probs {
  std::string name;
  long        freq;
  double      prob;
  bool operator<(const distr_probs& rhs) const {
    return prob > rhs.prob;
  }
};
int multi_dist( Logfile& l, Config& c ) {
  l.log( "multi_dist" );
  const std::string& filename         = c.get_value( "filename" );
  const std::string& lexicon_filename = c.get_value( "lexicon" );
  const std::string& counts_filename  = c.get_value( "counts" );
  const std::string& kvs_filename     = c.get_value( "kvs" );
  int                topn             = stoi( c.get_value( "topn", "1" ) );
  std::string        output_filename  = filename + ".md";

  Timbl::TimblAPI   *My_Experiment;
  std::string        distrib;
  std::vector<std::string> distribution;
  std::string        result;
  double             distance;

  l.inc_prefix();
  l.log( "filename:   "+filename );
  l.log( "lexicon:    "+lexicon_filename );
  l.log( "counts:     "+counts_filename );
  l.log( "kvs:        "+kvs_filename );
  l.log( "topn:       "+to_str(topn) );
  l.log( "OUTPUT:     "+output_filename );
  l.dec_prefix();

  std::ifstream file_in( filename.c_str() );
  if ( ! file_in ) {
    l.log( "ERROR: cannot load inputfile." );
    return -1;
  }
  std::ofstream file_out( output_filename.c_str(), std::ios::out );
  if ( ! file_out ) {
    l.log( "ERROR: cannot write output file." );
    file_in.close();
    return -1;
  }

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

  // read kvs
  // in multi_dist we don't need the size, we read a testfile instead.
  //
  std::vector<Classifier*> cls;
  std::vector<Classifier*>::iterator cli;
  int classifier_count = 0;
  if ( kvs_filename != "" ) {
    l.log( "Reading classifiers." );
    std::ifstream file_kvs( kvs_filename.c_str() );
    if ( ! file_kvs ) {
      l.log( "ERROR: cannot load kvs file." );
      return -1;
    }
    read_classifiers_from_file( file_kvs, cls );
    l.log( to_str((int)cls.size()) );
    file_kvs.close();
    for ( cli = cls.begin(); cli != cls.end(); cli++ ) {
      l.log( (*cli)->id );
      (*cli)->init();
      ++classifier_count;
    }
    l.log( "Read classifiers. Starting classification." );
  }

  std::vector<std::string>::iterator vi;
  std::ostream_iterator<std::string> output( file_out, " " );

  std::string a_line;
  std::vector<std::string> targets;
  std::vector<std::string>::iterator ri;
  const Timbl::ValueDistribution *vd;
  const Timbl::TargetValue *tv;
  std::vector<std::string> words;
  std::vector<distr_probs> distr_vec;
  std::vector<distr_probs>::iterator dei;
  std::map<std::string, double> combined_distr;
  std::map<std::string, double>::iterator mi;
  std::vector<double> distr_counts(classifier_count); // to calculate probs

  while( std::getline( file_in, a_line )) { // in right instance format

    words.clear();
    Tokenize( a_line, words, ' ' );
    std::string target = words.at( words.size()-1 );

    file_out << a_line << " [";

    // We loop over classifiers. We need a summed_distribution.
    //
    int classifier_idx = 0;
    for ( cli = cls.begin(); cli != cls.end(); cli++ ) {

      Classifier *classifier = *cli;
      Timbl::TimblAPI *timbl = classifier->get_exp();

      // get a line from the file, above cls loop!

      std::string cl = a_line;
      
      tv = timbl->Classify( cl, vd );
      std::string answer = tv->Name();

      int cnt = vd->size(); // size of distr.
      int distr_count = vd->totalSize(); // sum of freqs in distr.
      distr_counts[classifier_idx] = distr_count;

      /*l.log( (*cli)->id + ":" + cl + "/" + answer + " " + 
	to_str(cnt) + "/" + to_str(distr_count) );*/
      //file_out << " " << (*cli)->id << ":" << answer;
      file_out << " " << answer;

      Timbl::ValueDistribution::dist_iterator it = vd->begin();      
      while ( it != vd->end() ) {
	std::string tvs  = it->second->Value()->Name();
	long        wght = it->second->Weight(); // absolute frequency.
	double      p    = (double)wght / (double)distr_count;

	combined_distr[tvs] += p;

	if ( tvs == answer ) {
	  file_out << " " << p;
	}
	if ( tvs == target ) {
	  //was somewhere in target, keep score per classifier?
	}

	++it;
      }
      ++classifier_idx;
    } // classifiers
    
    file_out << " ] {";

    // sort combined, get top-n, etc.
    //
    for ( mi = combined_distr.begin(); mi != combined_distr.end(); mi++ ) {
      distr_probs  d;
      d.name   = mi->first;
      d.prob   = mi->second; // was d.freq
      distr_vec.push_back( d );
    }

    sort( distr_vec.begin(), distr_vec.end() );

    int cntr = topn;
    std::string combined_guess = "";
    dei = distr_vec.begin();
    while ( dei != distr_vec.end() ) { 
      if ( --cntr >= 0 ) {
	//file_out << (*dei).name << ' ' << (*dei).freq << ' ';
	//l.log( (*dei).name + ' ' + to_str((*dei).prob) ); // was .freq
	file_out << " " << (*dei).name << " " << (*dei).prob;
	if ( cntr == 0 ) {
	  combined_guess = (*dei).name;
	}
      }
      dei++;
    }
    file_out << " }";
    if ( target == combined_guess ) {
      file_out << " 1";
    }
    file_out << std::endl;
    distr_vec.clear();
    combined_distr.clear();
  } // get_line

  //and sat down [ some time ] down 1
  //Elinor and Marianne [ her it ] Marianne 1

  file_out.close();
  file_in.close();

  c.add_kv( "filename", output_filename );
  l.log( "SET filename to "+output_filename );
  return 0;
}
#else
int multi_dist( Logfile& l, Config& c ) {
  l.log( "Timbl support not built in." );  
  return -1;
}
#endif

// ---------------------------------------------------------------------------
