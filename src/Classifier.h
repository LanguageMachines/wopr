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

  void set_timbl( std::string t ) {
    timbl = t;
  }

  void init() {
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

};

#endif

