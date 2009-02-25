// Include STL string type

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <map>
#include <vector>
#include <algorithm>
#include <iterator>

#include <math.h>
#include <unistd.h>
#include <stdio.h>

#include "qlog.h"
#include "util.h"
#include "Config.h"
#include "runrunrun.h"
#include "server.h"
#include "generator.h"

//man 3 rand
//#include <openssl/rand.h>

#ifdef TIMBL
# include "timbl/TimblAPI.h"
#endif

/*
durian:wopr pberck$ ./wopr -r generate -p start:"Mexico has not",ibasefile:test/reuters.martin.tok.1000.ws3.ibase,timbl:"-a1 +D",len:10
*/
#ifdef TIMBL
int generate( Logfile& l, Config& c ) {
  l.log( "generate" );
  const std::string& start            = c.get_value( "start", "" );
  const std::string& filename         = c.get_value( "filename" );
  const std::string& ibasefile        = c.get_value( "ibasefile" );
  const std::string& timbl            = c.get_value( "timbl" );
  const std::string& end              = c.get_value( "end", " " );
  int                ws               = stoi( c.get_value( "ws", "3" ));
  bool               to_lower         = stoi( c.get_value( "lc", "0" )) == 1;
  bool               show_counts      = stoi( c.get_value( "sc", "0" )) == 1;
  std::string        output_filename  = filename + ".gen";
  int                len              = stoi( c.get_value( "len", "50" ) );
  int                n                = stoi( c.get_value( "n", "10" ) );
  Timbl::TimblAPI   *My_Experiment;
  std::string        distrib;
  std::vector<std::string> distribution;
  double             distance;

  l.inc_prefix();
  l.log( "filename:   "+filename );
  l.log( "ibasefile:  "+ibasefile );
  l.log( "timbl:      "+timbl );
  l.log( "ws:         "+to_str(ws) );
  l.log( "end:        "+end ); // end marker of sentences
  l.log( "n:          "+to_str(n) ); // number of sentences
  l.log( "len:        "+to_str(len) ); // max length of sentences
  l.log( "sc:         "+to_str(show_counts) );
  l.log( "lowercase:  "+to_str(to_lower) );
  l.log( "OUTPUT:     "+output_filename );
  l.dec_prefix();

  try {
    My_Experiment = new Timbl::TimblAPI( timbl );
    if ( ! My_Experiment->Valid() ) {
      l.log( "Timbl Experiment is not valid." );
      return 1;      
    }
    (void)My_Experiment->GetInstanceBase( ibasefile );
    if ( ! My_Experiment->Valid() ) {
      l.log( "Timbl Experiment is not valid." );
      return 1;
    }
    // My_Experiment->Classify( cl, result, distrib, distance );
  } catch (...) {
    l.log( "Cannot create Timbl Experiment..." );
    return 1;
  }
  l.log( "Instance base loaded." );

  std::ofstream file_out( output_filename.c_str(), std::ios::out );
  if ( ! file_out ) {
    l.log( "ERROR: cannot write output file." );
    return -1;
  }

  std::string a_line;
  std::string result;
  std::vector<std::string> results;
  std::vector<std::string> targets;
  std::vector<std::string>::iterator ri;
  const Timbl::ValueDistribution *vd;
  const Timbl::TargetValue *tv;
  std::vector<std::string> words;
  int correct = 0;
  int wrong   = 0;
  int correct_unknown = 0;
  int correct_distr = 0;
  Timbl::ValueDistribution::dist_iterator it;
  int cnt;
  int distr_count;

  // srand and rand() for debian?!
  srandom( (unsigned)now() ); 

  while ( --n >= 0 ) {
    a_line = start;
    if ( start == "" ) { // or less words than ws!
      for ( int i = 0; i < ws; i++ ) {
	a_line = a_line + "_ "; // could pick a random something
      }
      result = start;
    } else{
      result = start + " ";
    }
    int s_len = len;
    int no_choice = 0;
    int total_choices = 0;
    // abf, average branching factor?

    words.clear();
    Tokenize( a_line, words, ' ' ); // if less than ws, add to a_line

    // Make a function out of this.
    //
    //std::string foo = generate_one( c, a_line, len, ws, My_Experiment );
    //l.log( foo );

    while ( --s_len >= 0 ) { // Or till we hit a "."
      a_line = a_line + " _";
      tv = My_Experiment->Classify( a_line, vd );
      std::string answer = "";// tv->Name();
      cnt = vd->size();
      distr_count = vd->totalSize();

      if ( cnt == 1 ) {
	++no_choice;
      }
      total_choices += cnt;

      int rnd_idx = random() % cnt;
      //l.log( to_str(rnd_idx)+"/"+to_str(cnt) );
      
      // Take (top) answer, or choose something from the
      // distribution.
      //
      it = vd->begin();
      for ( int i = 0; i < rnd_idx; i++ ) {
	std::string tvs  = it->second->Value()->Name();
	++it;
      }
      std::string tvs  = it->second->Value()->Name();
      double      wght = it->second->Weight();
      answer = tvs;
      
      result = result + answer;
      if ( show_counts && ( cnt > 1 )) { // Show if more than one choice
	result = result + "("+to_str(cnt)+")";
      }
      result = result + " ";

      // shift/add/repeat
      //
      copy( words.begin()+1, words.end(), words.begin() );
      words.at(ws-1) = answer;

      a_line = "";
      for ( int i = 0; i < ws; i++ ) {
	a_line = a_line + words[i] + " ";
      }

      // Stop when "."
      //
      if ( tvs == end ) {
	s_len = 0;
      }
    }
    
    //l.log( result );
    file_out << result;
    if ( show_counts) {
      file_out << " " << no_choice << " " << total_choices;
    }
    file_out << std::endl;
  }
  file_out.close();

  return 0;
}  
#else
int generate( Logfile& l, Config& c ) {
  l.log( "No TIMBL support." );
  return -1;
}  
#endif

#ifdef TIMBL
// returns one sentence of length len.
//
std::string generate_one( Config& c, std::string& a_line, int len, int ws,
			  Timbl::TimblAPI* My_Experiment ) {

  std::string result = "";
  std::vector<std::string>::iterator ri;
  const Timbl::ValueDistribution *vd;
  const Timbl::TargetValue *tv;
  std::vector<std::string> words;
  Timbl::ValueDistribution::dist_iterator it;
  int cnt;

  words.clear();
  Tokenize( a_line, words, ' ' ); // if less than ws, add to a_line

  while ( --len >= 0 ) {
    a_line = a_line + " _";
    tv = My_Experiment->Classify( a_line, vd );
    std::string answer = "";// tv->Name();
    cnt = vd->size();
    
    int rnd_idx = random() % cnt;
    
    // Take (top) answer, or choose something from the
    // distribution.
    //
    it = vd->begin();
    for ( int i = 0; i < rnd_idx; i++ ) {
      std::string tvs  = it->second->Value()->Name();
      ++it;
    }
    std::string tvs  = it->second->Value()->Name();
    double      wght = it->second->Weight();
    answer = tvs;
    
    result = result + answer + " ";
    
    // shift/add/repeat
    //
    copy( words.begin()+1, words.end(), words.begin() );
    words.at(ws-1) = answer;
    
    a_line = "";
    for ( int i = 0; i < ws; i++ ) {
      a_line = a_line + words[i] + " ";
    }  
  }

  return result;
  
}
#endif


#ifdef TIMBL
// This one listens on a socket, and uses generate_one
//
int generate_server( Logfile& l, Config& c ) {
  l.log( "generate_server" );
  const std::string& start            = c.get_value( "start", "" );
  const std::string& ibasefile        = c.get_value( "ibasefile" );
  const std::string& timbl            = c.get_value( "timbl" );
  const std::string& end              = c.get_value( "end", " " );
  int                ws               = stoi( c.get_value( "ws", "3" ));
  bool               to_lower         = stoi( c.get_value( "lc", "0" )) == 1;
  int                len              = stoi( c.get_value( "len", "50" ) );
  int                n                = stoi( c.get_value( "n", "10" ) );
  Timbl::TimblAPI   *My_Experiment;

  l.inc_prefix();
  l.log( "ibasefile:  "+ibasefile );
  l.log( "timbl:      "+timbl );
  l.log( "ws:         "+to_str(ws) );
  l.log( "end:        "+end ); // end marker of sentences
  l.log( "n:          "+to_str(n) ); // number of sentences
  l.log( "len:        "+to_str(len) ); // max length of sentences
  l.log( "lowercase:  "+to_str(to_lower) );
  l.dec_prefix();

  try {
    My_Experiment = new Timbl::TimblAPI( timbl );
    if ( ! My_Experiment->Valid() ) {
      l.log( "Timbl Experiment is not valid." );
      return 1;      
    }
    (void)My_Experiment->GetInstanceBase( ibasefile );
    if ( ! My_Experiment->Valid() ) {
      l.log( "Timbl Experiment is not valid." );
      return 1;
    }
  } catch (...) {
    l.log( "Cannot create Timbl Experiment..." );
    return 1;
  }
  l.log( "Instance base loaded." );

  std::string a_line;
  std::string result;

  // srand and rand() for debian?!
  srandom( (unsigned)now() ); 

  while ( --n >= 0 ) {
    a_line = start;
    if ( start == "" ) { // or less words than ws!
      for ( int i = 0; i < ws; i++ ) {
	a_line = a_line + "_ "; // could pick a random something?
      }
    }

    // Make a function out of this.
    //
    std::string foo = generate_one( c, a_line, len, ws, My_Experiment );
    // We should add foo to start if we want to use start...
    if ( start != "" ) {
      foo = start + " " + foo;
    }
    l.log( foo );
    
  }

  return 0;
}  
#else
int generate_server( Logfile& l, Config& c ) {
  l.log( "No TIMBL support." );
  return -1;
}  
#endif
