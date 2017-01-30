#ifndef _CLASSIFIER_H
#define _CLASSIFIER_H

#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <vector>
#include <algorithm>

#include <config.h>

#ifdef TIMBL
# include "timbl/TimblAPI.h"
#endif

#include "wopr/util.h"
#include "wopr/ngrams.h"

struct md2_elem {
  std::string token;
  long        freq;
  double      prob;
  bool operator<(const md2_elem& rhs) const {
    return prob > rhs.prob;
  }
  bool operator==(const md2_elem& rhs) const {
    return token == rhs.token;
  }
};

struct md2 {
md2():
  prob(0.0),
    cnt(0),
    distr_count(0),
    entropy(0.0),
    md(0),
    mal(false),
    info(0),
    in_distr(false),
    idx(0),
    mrr(0.0)
  {};
  std::string name;
  std::string cl;
  std::string answer;
  double      prob;
  std::string target;
  long        cnt;
  long        distr_count;
  double      entropy;
  size_t      md;
  bool        mal;
  int         info; // 0:no info, 1:in distr, 2:correct
  bool        in_distr; // needed for spelcorr without changing old functionality
  long        idx; // index in distribution
  double      mrr;
  std::vector<md2_elem> distr;
};

const unsigned int INFO_NOINFO   = 0;
const unsigned int INFO_INDISTR  = 1;
const unsigned int INFO_UNKNOWN  = 2;
const unsigned int INFO_CORRECT  = 4;
const unsigned int INFO_WRONG    = 8;

class cmp {
 public:
  bool operator() (const long x, const long y) {
    return (x-y) > 0;
  }
};

class Classifier {

 private:
  std::ifstream file_in;

 public:
  std::string id;
  int         type;
  int         subtype;
  std::string ibasefile;
  int         ws;
  std::string gatetrigger;
  int         gatepos;
  std::string timbl;
  std::string testfile;
  std::string distfile;
  double      weight;
  double      distprob;
  long        correct;
  long        score_cg;
  long        score_cd;
  long        score_ic;
  long        classification_count;
  md2         classification;
  std::vector<std::string> words;
#ifdef TIMBL
  Timbl::TimblAPI       *My_Experiment;
  const Timbl::ValueDistribution *vd;
  const Timbl::TargetValue *tv;
#endif

  //! Constructor.
  //!
  explicit Classifier( const std::string& );
  Classifier( const std::string&, int );

  //! Destructor.
  //!
  ~Classifier();

  void set_ibasefile( const std::string& f ) {
    ibasefile = f;
  }

  void set_ws( int i ) {
    ws = i;
  }
  int get_ws() {
    return ws;
  }

  void set_timbl( const std::string& t ) {
    timbl = t;
  }

  void set_testfile( const std::string& t ) {
    testfile = t;
  }
  const std::string& get_testfile() {
    return testfile;
  }

  void set_distfile( const std::string& t ) {
    distfile = t;
  }
  const std::string& get_distfile() {
    return distfile;
  }

  void set_weight( double w ) {
    weight = w;
  }
  double get_weight() {
    return weight;
  }

  void set_distprob( double p ) {
    distprob = p;
  }
  double get_distprob() {
    return distprob;
  }

  void set_type( int t ) {
    type = t;
  }
  int get_type() {
    return type;
  }

  void set_subtype( int t ) {
    subtype = t;
  }
  int get_subtype() {
    return subtype;
  }

  void set_gatetrigger( const std::string& t ) {
    gatetrigger = t;
  }
  const std::string& get_gatetrigger() {
    return gatetrigger;
  }

  void set_gatepos( int p ) {
    gatepos = p;
  }
  int get_gatepos() {
    return gatepos;
  }

  bool open_file() {
    if ( type == 1 ) {
      file_in.open( testfile.c_str() );
    } else if ( type == 2 ) {
      file_in.open( distfile.c_str() );
    } else if ( type == 3 ) {
      file_in.open( testfile.c_str() );
    } else if ( type == 4 ) {
      file_in.open( testfile.c_str() );
    }
    return file_in.is_open();
  }

  // Depending on type...
  //
  bool classify_next() {
    if ( std::getline( file_in, classification.cl ) ) {
      last_word( classification.cl, classification.target );
      classification.distr.clear();

      if ( type == 1 ) { // A "real" classifier
#ifdef TIMBL
	tv = My_Experiment->Classify( classification.cl, vd );
	if ( ! tv ) {
	  return false;
	}
	classification.answer = tv->Name();

	classification.md  = My_Experiment->matchDepth();
	classification.mal = My_Experiment->matchedAtLeaf();

	++classification_count;
	classification.cnt = vd->size(); // size of distr.
	classification.distr_count = vd->totalSize(); // sum of freqs in distr.

	Timbl::ValueDistribution::dist_iterator it = vd->begin();
	while ( it != vd->end() ) {
	  std::string tvs  = it->second->Value()->Name();
	  long        wght = it->second->Weight(); // absolute frequency.
	  double      p    = (double)wght / (double)vd->totalSize();
	  md2_elem md2e;
	  md2e.token = tvs;
	  md2e.freq = wght;
	  md2e.prob = p;
	  classification.distr.push_back( md2e );
	  if ( tvs == classification.answer ) {
	    classification.prob = p; // in the distr.
	  }
	  ++it;
	}

	if ( classification.answer == classification.target ) {
	  ++correct;
	}
#endif
      } else if ( type == 2 ) {
	// Probabilities from...?
	words.clear();

	// Remove target, tokenize.
	//
	but_last_word( classification.cl, classification.cl );
	Tokenize( classification.cl, words, ' ' );

	classification.answer = "DIST";
	classification.md  = 0;
	classification.mal = false;
	classification.cnt = 0;
	classification.distr_count = words.size();

	++classification_count;

	std::vector<std::string>::iterator it;
	std::vector<md2_elem>::iterator itf;

	// Make the distribution unique? Or do we let the frequency
	// affect the distribution as well (probs will be added)?
	//
	// Make configurable...?
	//
	//sort( words.begin(), words.end() ); // sorting changes order...
	//it = unique (words.begin(), words.end());
	//words.resize( it - words.begin() );

	it = words.begin();
	while ( it != words.end() ) {
	  std::string tvs  = *it;
	  if ( tvs != "_" ) { // check here for if it is a double?
	    long        wght = 1;// ?!
	    double      p    = distprob; // ?!
	    md2_elem md2e;
	    md2e.token = tvs;
	    md2e.freq = wght;
	    md2e.prob = p;
	    //
	    // Check for doubles.
	    //
	    itf = find(classification.distr.begin(), classification.distr.end(), md2e);
	    if ( itf == classification.distr.end() ) { // not double
	      classification.distr.push_back( md2e );
	      if ( tvs == classification.answer ) {
		classification.prob = p;
	      }
	    }
	  }
	  ++it;
	} //while

      } //type == 2

      return true;
    } else {
      return false;
    }
  }

  // For the gated classifiers.
  //
  bool classify_one( const std::string& a_line ) {

    classification.cl = a_line;
    last_word( classification.cl, classification.target );

    classification.distr.clear();
    classification.prob = 0;

#ifdef TIMBL
    tv = My_Experiment->Classify( classification.cl, vd );
    if ( ! tv ) {
      return false;
    }

    classification.info     = INFO_WRONG; // unless proven wrong :)
    classification.in_distr = false;

    classification.answer = tv->Name();

    classification.md  = My_Experiment->matchDepth();
    classification.mal = My_Experiment->matchedAtLeaf();

    ++classification_count;
    classification.cnt = vd->size(); // size of distr.
    classification.distr_count = vd->totalSize(); // sum of freqs in distr.
    classification.entropy = 0.0;

    // Timbl distr is unsorted...
    Timbl::ValueDistribution::dist_iterator it = vd->begin();
    long classification_freq = 0.0;
    std::map<long, long, std::greater<long> > dfreqs;
    /*
      To get MRR, make hash with freq->count.
      In the end, we know freq of answer, look up in hash.

      130->1      answer in dist. has freq 100, "3rd place".
      120->1      sort hash on key. Reversed.
      100->3
       80->1
    */
    // Should calc entropy and other stuff relevant for spel corr here.
    //
    while ( it != vd->end() ) {
      std::string tvs  = it->second->Value()->Name();
      long        wght = it->second->Weight(); // absolute frequency.
      double      p    = (double)wght / (double)vd->totalSize();
      md2_elem md2e;
      md2e.token = tvs;
      md2e.freq = wght;
      md2e.prob = p;
      dfreqs[wght] += 1;
      classification.distr.push_back( md2e );
      if ( tvs == classification.target ) {
	classification.prob = p; // in the distr.
	classification.info = INFO_INDISTR;
	classification.in_distr = true;
	classification_freq = wght;
	//break
      }
      classification.entropy -= ( p * log2(p) );
      ++it;
    }

    long idx = 1;
    classification.idx = 0;
    classification.mrr = 0.0;
    std::map<long, long>::iterator dfi = dfreqs.begin();
    while ( dfi != dfreqs.end() ) {
      if ( dfi->first == classification_freq ) {
	classification.idx = idx;
	classification.mrr = (double)1.0/idx;
      }
      //if ( idx > some_limit ) { break;}
      ++dfi;
      ++idx;
    }

    // count correct not as in_distr...
    //
    if ( classification.answer == classification.target ) {
      ++correct;
      classification.info = INFO_CORRECT;
      ++score_cg;
    }
    if ( classification.info == INFO_INDISTR ) {
      ++score_cd;
    } else if ( classification.info == INFO_WRONG ) {
      ++score_ic;
    }
#endif
    return true;
  }

  void close_file() {
    file_in.close();
  }

  void inc_correct() {
    ++correct;
  }
  long get_correct() {
    return correct;
  }

  long get_cc() {
    return classification_count;
  }

  long get_cg() {
    return score_cg;
  }
  long get_cd() {
    return score_cd;
  }
  long get_ic() {
    return score_ic;
  }


  std::string info_str() {
    return id+"/"+ibasefile+"/"+testfile+"/wgt="+to_str(weight)
      +"/type="+to_str(type);
  }

  void init() {
    correct = 0;
    score_cg = 0;
    score_cd = 0;
    score_ic = 0;
    classification_count = 0;
#ifdef TIMBL
    if ( (type == 1) || (type == 3) || (type == 4) ) { //  maybe !=2
      My_Experiment = new Timbl::TimblAPI( timbl );
      (void)My_Experiment->GetInstanceBase( ibasefile );
    }
#endif
  }

#ifdef TIMBL
  Timbl::TimblAPI *get_exp() {
    return My_Experiment;
  }
#endif

/*
transform( result.begin(), result.end(), vectors[i].begin(),result.begin(), plus<T>() ) ;
*/
};

#endif
