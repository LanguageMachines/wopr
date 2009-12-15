#ifndef _CLASSIFIER_H
#define _CLASSIFIER_H

#include <string>
#ifdef TIMBL
# include "timbl/TimblAPI.h"
#endif

class Classifier {

 public:
  std::string            id;
  std::string            ibasefile;
  int                    ws;
  std::string            timbl;
  long                   correct; 
#ifdef TIMBL
  Timbl::TimblAPI       *My_Experiment;
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

  void set_timbl( std::string t ) {
    timbl = t;
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

