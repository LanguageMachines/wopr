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

class History {

 public:
  std::vector<Context*> his;
  size_t max;

  //! Constructor.
  //!
  History(size_t m) {
    max = m;
    his.resize(max);
    his.clear();
  };

  //! Destructor.
  //!
  ~History() {};

  void add(Context *c) {
    // check size? resize if too big?
    if ( his.size() < max ) {
      his.push_back( new Context(c) );
    } else {
      copy( his.begin()+1, his.end(), his.begin() );
      his.pop_back();
      his.push_back( new Context(c) );
    }
    //dump();
  }
  
  Context* get() {
    // check size?
    std::cerr << the_time() << ": (get)his.size()=" << his.size() << std::endl;
    if ( his.size() == 0 ) {
      return NULL;
    }
    Context* answer = his.at( his.size()-1 );
    his.pop_back();
    //dump();
    return answer;
  }

  void clear() {
    his.clear();
  }

  void dump() {
    for ( int i = 0; i < his.size(); i++ ) {
      std::cerr << the_time() << ": (dump)(" << i << ")=" << his.at(i)->toString() << std::endl;
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
  time_t start_t;
  time_t last_t;
  int lpos; // counts letters after space

  PDTC *ltr_c;
  PDTC *wrd_c;

  Context *ltr_ctx;
  Context *wrd_ctx;

  Context *p_wrd_ctx;

  History *ltr_his; // previous
  History *wrd_his; // contexts

  std::string wrd_current;

  //! Constructor.
  //!
  PDT() {
    start_t = utc();
    last_t = utc();
    lpos = 0;
  };

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
    ltr_his = new History(20);
    touch();
  }

  // Specify the word classifier.
  //
  void set_wrd_c( PDTC *c ) {
    wrd_c = c;
    wrd_ctx = new Context( c->ctx_size );
    wrd_his = new History(4);
    touch();
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
    touch();
  }

  // Set the letter depth(s), only one number, but we still
  // need the vector for generate_tree().
  //
  void set_ltr_dl( std::string s ) {
    dl = stoi( s, 32 );
    ltr_depths.clear();
    ltr_depths.resize( 2, 1 );
    ltr_depths.at(1) = dl;
    touch();
  }

  void clear() {
    ltr_ctx->reset();
    wrd_ctx->reset();
    ltr_his->clear();
    wrd_his->clear();
    wip.clear();
    lpos = 0;
    touch();
  }

  // Check if we have undo history
  //
  int get_ltr_his() {
    return ltr_his->his.size();
    touch();
  }

  // Assume one letter, or explode it first?
  // Adding ("typing") a letter involves adding it
  // to the letter context.
  // A space should signal a word...
  // Word context should have a "current word" which builds
  // up the word simultaneously? (backspaces?)
  //
  void add_ltr( std::string& s ) {
    if ( (s.length() == 2) && (s.at(0) == '\\') ) {
      // Extremely crude method to get \' to '
      s = s.at(1);
    }
    ltr_his->add( ltr_ctx ); // add current context to history
    ltr_ctx->push( s );
    //std::cerr << ltr_ctx->toString() << std::endl;
    wip = wip + s;
    ++lpos;
    touch();
  }

  // Pitfalls: >1 space
  //
  void add_spc() {
    if ( ltr_ctx->last_letter() == "_" ) {
      //
      // Ignore dubbel spaces for now.
      //
      touch();
      return;
    }
    ltr_his->add( ltr_ctx );
    ltr_ctx->push( "_" );
    // now add wip to the words?
    wrd_his->add( wrd_ctx );
    add_wrd( wip );
    pwip = wip; // for "backspace" &c?
    wip.clear();
    lpos = 0;
    touch();
  }

  // Delete the last letter. What to do with word context?
  // wip must also be adjusted.
  //
  void del_ltr() {
    //std::cerr << ltr_ctx->last_letter() << std::endl;
    if ( ltr_ctx->last_letter() == "_" ) { //and lastlast != " " ?
      // fix word context? this needs to fix "wip" as well,
      // otherwise we miss letters in the beginning
      add_log( "fix" );
      Context *pw = wrd_his->get(); //removes also
      if ( pw != NULL ) {
	wrd_ctx->cp( pw );
      }
    } else {
      // take from wip.
      //
      if ( wip.length() > 0 ) {
	wip = wip.substr(0, wip.length()-1);
      }
      --lpos;
    }
    Context *pc = ltr_his->get(); //removes also
    if ( pc != NULL ) {
      ltr_ctx->cp( pc );
    }
    touch();
  }

  // For generation, we need this at least.
  //
  void add_wrd( std::string& s ) {
    wrd_ctx->push( s );
    //std::cerr << "(" << wrd_ctx->toString() << ")" << std::endl;
    touch();
  }

  void ltr_generate( std::vector<std::string>& res ) {
    std::string t;
    generate_tree( ltr_c->My_Experiment, (Context&)(*ltr_ctx), res, 1, ltr_depths, 1, t );
    touch();
  }

  void wrd_generate( std::vector<std::string>& res ) {
    std::string t;
    generate_tree( wrd_c->My_Experiment, (Context&)(*wrd_ctx), res, n, wrd_depths, n, t );
    touch();
  }

  // "Advance" this string in the system. Could be a predicted string.
  // We could be half way a word, the first word is the desired
  // continuation. Or we could be on a space.
  //
  void feed( std::string& s ) {
    // feed_ltr( std::string& s )
    // if ( ltr_ctx->last_letter() == "_" ) ...
    // feed_wrd( std::string& s )
  }



  time_t age() {
    return utc() - start_t;
    touch();
  }
  time_t idle() {
    return utc() - last_t;
  }
  void touch() {
    last_t = utc();
  }

};


#endif
