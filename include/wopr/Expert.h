#ifndef _EXPERT_H
#define _EXPERT_H

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

#include "util.h"
#include "ngrams.h"

class Expert {

 public:
  std::string id;
  int         type;
  std::string ibasefile;
  std::string trigger;
  int         offset;
  std::string timbl;
  int         called;
  std::string highest;
  int         highest_f;
#ifdef TIMBL
  Timbl::TimblAPI                *My_Experiment;
  const Timbl::ValueDistribution *vd;
  const Timbl::TargetValue       *tv;
#endif

  //! Constructor.
  //!
  Expert( const std::string& n ) {
	id     = n;
	type   = 1;
	called = 0;
  }

  Expert( const std::string& n, int t ) {
	id     = n;
	type   = t;
	called = 0;
  }

  //! Destructor.
  //!
  ~Expert();

  void set_ibasefile( const std::string& f ) {
    ibasefile = f;
  }

  void set_timbl( const std::string& t ) {
    timbl = t;
  }

  void set_type( int t ) {
    type = t;
  }
  int get_type() {
    return type;
  }

  void set_trigger( const std::string& t ) {
    trigger = t;
  }
  const std::string& get_trigger() {
    return trigger;
  }

  void set_highest( const std::string& h ) {
    highest = h;
  }
  const std::string& get_highest() {
    return highest;
  }

  void set_highest_f( int h ) {
    highest_f = h;
  }
  int get_highest_f() {
    return highest_f;
  }

  void set_offset( int p ) {
    offset = p;
  }
  int get_offsetpos() {
    return offset;
  }

  void call() {
	called += 1;
  }
  int get_called() {
	return called;
  }

  void init() {
#ifdef TIMBL
	try {
	  My_Experiment = new Timbl::TimblAPI( timbl );
	  (void)My_Experiment->GetInstanceBase( ibasefile );
	} catch (...) {
	}
#endif
  }

#ifdef TIMBL
  Timbl::TimblAPI *get_exp() {
    return My_Experiment;
  }
#endif

};

#endif

