#ifndef _CLASSIFIER_H
#define _CLASSIFIER_H

#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <vector>

#ifdef TIMBL
# include "timbl/TimblAPI.h"
#endif

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
  std::vector<md2_elem> distr;
};

class Classifier {

 private:
  std::ifstream file_in;

 public:
  std::string id;
  std::string ibasefile;
  int         ws;
  std::string timbl;
  std::string testfile;
  long        correct; 
  long        classification_count; 
  md2         classification;
#ifdef TIMBL
  Timbl::TimblAPI       *My_Experiment;
  const Timbl::ValueDistribution *vd;
  const Timbl::TargetValue *tv;
#endif

  //! Constructor.
  //!
  Classifier( const std::string& );

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

  void open_file() {
    file_in.open( testfile.c_str() );
    std::cerr << "Opened file: " << testfile << std::endl;
  }

  bool classify_next() {
    if ( std::getline( file_in, classification.cl ) ) {
      last_word( classification.cl, classification.target );
      classification.distr.clear();
#ifdef TIMBL    
      tv = My_Experiment->Classify( classification.cl, vd );
      classification.answer = tv->Name();
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

  void init() {
    correct = 0;
#ifdef TIMBL
    My_Experiment = new Timbl::TimblAPI( timbl );
    (void)My_Experiment->GetInstanceBase( ibasefile );
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
};

#endif

