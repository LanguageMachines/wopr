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
  std::vector<md2_elem> distr;
};

class Classifier {

 private:
  std::ifstream file_in;

 public:
  std::string id;
  int         type;
  std::string ibasefile;
  int         ws;
  std::string timbl;
  std::string testfile;
  std::string distfile;
  double      weight;
  double      distprob;
  long        correct; 
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

  bool open_file() {
    if ( type == 1 ) {
      file_in.open( testfile.c_str() );
    } else if ( type == 2 ) {
      file_in.open( distfile.c_str() );
    }
    return file_in.is_open();
  }

  // Depending on type...
  //
  bool classify_next() {
    if ( std::getline( file_in, classification.cl ) ) {
      last_word( classification.cl, classification.target );
      classification.distr.clear();
      // Here we could discern type. target we have in gc data also.
      if ( type == 1 ) {
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
	    classification.prob = p;
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

	// Make the distribution unique? Or do we let the frequency
	// affect the distribution as well (probs will be added)?
	//
	sort( words.begin(), words.end() );
	it = unique (words.begin(), words.end());
	words.resize( it - words.begin() );

	it = words.begin();      
	while ( it != words.end() ) {
	  std::string tvs  = *it;
	  if ( tvs != "_" ) {
	    long        wght = 1;// ?!
	    double      p    = distprob; // ?!
	    md2_elem md2e;
	    md2e.token = tvs;
	    md2e.freq = wght;
	    md2e.prob = p;
	    classification.distr.push_back( md2e );
	    if ( tvs == classification.answer ) {
	      classification.prob = p;
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

  void close_file() {
    file_in.close();
  }

  void inc_correct() {
    ++correct;
  }
  long get_correct(){
    return correct;
  }

  long get_cc(){
    return classification_count;
  }

  std::string info_str() {
    return id+"/"+ibasefile+"/"+testfile+"/"+to_str(weight);
  }

  void init() {
    correct = 0;
#ifdef TIMBL
    if ( type == 1 ) {
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

