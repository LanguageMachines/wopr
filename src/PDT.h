#ifndef _PDT_H
#define _PDT_H

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

const int PDTC_EMPTY   = 0;
const int PDTC_INVALID = 1;

class PDTC {
 public:
  std::string id;
  int         type;
  int         status;
  std::string ibasefile;
  std::string timbl;
  std::vector<std::string> log;

#ifdef TIMBL
  Timbl::TimblAPI *My_Experiment;
  const Timbl::ValueDistribution *vd;
  const Timbl::TargetValue *tv;
#endif

  Context *ctx;
  int      ctx_size;

  PDTC() {
    status = PDTC_EMPTY;
    add_log( "PDC() called." );
  }

  ~PDTC() {
    log.clear();
  }

  void init( const std::string f, const std::string t, int c ) {
    ibasefile = f;
    timbl = t;
    ctx_size = c;

    add_log( "init('"+f+"','"+t+"') started." );

    ctx = new Context( c );
    add_log( "new Context("+to_str(c)+")" );

    try {
      My_Experiment = new Timbl::TimblAPI( timbl );
      if ( ! My_Experiment->Valid() ) {
	status = PDTC_INVALID;
	add_log( "My_Experiment invalid." );
      }
      (void)My_Experiment->GetInstanceBase( ibasefile );
      if ( ! My_Experiment->Valid() ) {
	status = PDTC_INVALID;
	add_log( "My_Experiment invalid." );
      }
    } catch (...) {
      status = PDTC_INVALID;
      add_log( "My_Experiment invalid." );
    }
    add_log( "init() ready." );
  }

  void add_log( const std::string& s ) {
    std::cerr << the_time() << ": " << s << std::endl;
    log.push_back( s );
  }
  void dump_log() {
    std::vector<std::string>::iterator li;
    for ( li = log.begin(); li != log.end(); li++ ) {
      std::cerr << *li << "\n";
    }
  }
  
};

// ----

class PDT {
 public:
  std::string id;
  std::vector<std::string> log;
  std::string ds;
  std::vector<int> wrd_depths;
  int         n;
  int         dl;
  std::vector<int> ltr_depths;
  std::string wip; // word in progress
  std::string pwip; // previous word in progress

  PDTC *ltr_c;
  PDTC *wrd_c;

  Context *ltr_ctx;
  Context *wrd_ctx;

  std::string wrd_current;

  //! Constructor.
  //!
  PDT() {};

  //! Destructor.
  //!
  ~PDT() {
    log.clear();
  };

  // Debug/logging
  //
  void add_log( const std::string& s ) {
    std::cerr << the_time() << ": " << s << std::endl;
    log.push_back( s );
  }
  void dump_log() {
    std::vector<std::string>::iterator li;
    for ( li = log.begin(); li != log.end(); li++ ) {
      std::cerr << *li << "\n";
    }
  }

  // Specify the character classifier.
  //
  void set_ltr_c( PDTC *c ) {
    ltr_c = c;
    ltr_ctx = new Context( c->ctx_size );
  }

  // Specify the word classifier.
  //
  void set_wrd_c( PDTC *c ) {
    wrd_c = c;
    wrd_ctx = new Context( c->ctx_size );
  }

  // Set the word depth(s) and length n from s. Vector is
  // needed for generate_tree().
  //
  void set_wrd_ds( std::string s ) {
    ds = s;
    n = ds.length();
    wrd_depths.clear();
    wrd_depths.resize( n+1, 1 );
    for( int i = 0; i < n; i++) {
      wrd_depths.at(n-i) = stoi( ds.substr(i, 1), 32 ); // V=31
    }
  }

  // Set the letter depth(s), only one number, but we still
  // need the vector for generate_tree().
  //
  void set_ltr_dl( std::string s ) {
    dl = stoi( s, 32 );
    ltr_depths.clear();
    ltr_depths.resize( 2, 1 );
    ltr_depths.at(1) = dl;
  }

  // Assume one letter, or explode it first?
  // Adding ("typing") a letter involves adding it
  // to the letter context.
  // A space should signal a word...
  // Word context should have a "current word" which builds
  // up the word simultaneously? (backspaces?)
  //
  void add_ltr( std::string& s ) {
    ltr_ctx->push( s );
    //std::cerr << ltr_ctx->toString() << std::endl;
    wip = wip + s;
  }

  void add_spc() {
    ltr_ctx->push( "_" );
    // now add wip to the words?
    add_wrd( wip );
    pwip = wip; // for "backspace" &c?
    wip.clear();
  }

  // For generation, we need this at least.
  //
  void add_wrd( std::string& s ) {
    wrd_ctx->push( s );
    //std::cerr << "(" << wrd_ctx->toString() << ")" << std::endl;
  }

  void ltr_generate( std::vector<std::string>& res ) {
    std::string t;
    generate_tree( ltr_c->My_Experiment, (Context&)(*ltr_ctx), res, 1, ltr_depths, t );
  }

  void wrd_generate( std::vector<std::string>& res ) {
    std::string t;
    generate_tree( wrd_c->My_Experiment, (Context&)(*wrd_ctx), res, n, wrd_depths, t );
  }

};


#endif
