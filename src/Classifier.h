#ifndef _CLASSIFIER_H
#define _CLASSIFIER_H

#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <vector>
#include <algorithm>

#ifdef TIMBL
# include "timbl/TimblAPI.h"
#endif

#include "util.h"
#include "ngrams.h"

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
  std::string name;
  std::string cl;
  std::string answer;
  double      prob;
  std::string target;
  long        cnt;
  long        distr_count;
  size_t      md;
  bool        mal;
  int         info; // 0:no info, 1:in distr, 2:correct
  std::vector<md2_elem> distr;
};

const unsigned int INFO_NOINFO   = 0;
const unsigned int INFO_INDISTR  = 1;
const unsigned int INFO_UNKNOWN  = 2;
const unsigned int INFO_CORRECT  = 4;
const unsigned int INFO_WRONG    = 8;

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
  long        correct_distr;
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
  Classifier( const std::string& );
  Classifier( const std::string&, int );

  //! Destructor.
  //!
  ~Classifier();

  void set_ibasefile( const std::string f ) {
    ibasefile = f;
  }

  void set_ws( int i ) {
    ws = i;
  }
  int get_ws() {
    return ws;
  }

  void set_timbl( const std::string t ) {
    timbl = t;
  }

  void set_testfile( const std::string t ) {
    testfile = t;
  }
  const std::string& get_testfile() {
    return testfile;
  }

  void set_distfile( const std::string t ) {
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

  void set_gatetrigger( const std::string t ) {
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
  bool classify_one( std::string a_line ) {
    
    classification.cl = a_line;
    last_word( classification.cl, classification.target );

    classification.distr.clear();
    classification.prob = 0;

#ifdef TIMBL    
    tv = My_Experiment->Classify( classification.cl, vd );

    classification.info = INFO_WRONG; // unless proven wrong :)

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
      if ( tvs == classification.target ) {
	classification.prob = p; // in the distr.
	classification.info = INFO_INDISTR;
      }
      ++it;
    }
    
    if ( classification.answer == classification.target ) {
      ++correct;
      classification.info = INFO_CORRECT;
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

  std::string info_str() {
    return id+"/"+ibasefile+"/"+testfile+"/wgt="+to_str(weight)
      +"/type="+to_str(type);
  }

  void init() {
    correct = 0;
    correct_distr = 0;
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

#ifdef TIMBL
  void classify( const std::string& cl, const Timbl::TargetValue *tv,
		 const Timbl::ValueDistribution *vd ) {
    tv = My_Experiment->Classify( cl, vd );
  }
#endif

/*
transform( result.begin(), result.end(), vectors[i].begin(),result.begin(), plus<T>() ) ;
*/
};

#endif

