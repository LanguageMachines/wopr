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

struct md2_distr {
  std::string name;
  long        freq;
  double      prob;
  bool operator<(const md2_distr& rhs) const {
    return prob > rhs.prob;
  }
};
struct md2 {
  std::string name;
  std::string cl;
  std::string answer;
  long        cnt;
  long        distr_count;
  std::vector<md2_distr> distr;
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
  std::string cl;
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
    if ( std::getline( file_in, cl ) ) {
      classification.cl = cl;
#ifdef TIMBL    
      tv = My_Experiment->Classify( cl, vd );
      classification.answer = tv->Name();
      
      classification.cnt = vd->size(); // size of distr.
      classification.distr_count = vd->totalSize(); // sum of freqs in distr. 
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

  void init() {
    correct = 0;
#ifdef TIMBL
    My_Experiment = new Timbl::TimblAPI( timbl );
    (void)My_Experiment->GetInstanceBase( ibasefile );
#endif
    open_file();
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

