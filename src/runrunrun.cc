// ---------------------------------------------------------------------------
// $Id$
// ---------------------------------------------------------------------------

/*****************************************************************************
 * Copyright 2007 - 2009 Peter Berck                                         *
 *                                                                           *
 * This file is part of wopr.                                                *
 *                                                                           *
 * wopr is free software; you can redistribute it and/or modify it           *
 * under the terms of the GNU General Public License as published by the     *
 * Free Software Foundation; either version 2 of the License, or (at your    *
 * option) any later version.                                                *
 *                                                                           *
 * wopr is distributed in the hope that it will be useful, but WITHOUT       *
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or     *
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License      *
 * for more details.                                                         *
 *                                                                           *
 * You should have received a copy of the GNU General Public License         *
 * along with wpred; if not, write to the Free Software Foundation,          *
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA               *
 *****************************************************************************/

// ---------------------------------------------------------------------------
//  Includes.
// ---------------------------------------------------------------------------

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
#include <sys/stat.h> 

#include "qlog.h"
#include "util.h"
#include "Config.h"
#include "runrunrun.h"
#include "server.h"
#include "tag.h"
#include "Classifier.h"
#include "Multi.h"
#include "levenshtein.h"
#include "generator.h"
#include "lcontext.h"

#ifdef TIMBL
# include "timbl/TimblAPI.h"
#endif

// ---------------------------------------------------------------------------
// ./wpred --run run_exp2,run_external -e "sleep 10"
// ./wpred --run make_data --params filename:settings
// ./wpred --run timbl --params -- timbl:-a
// ./wpred --run make_wd --params filename:reuters.martin.tok,ws:8
//
// ./wopr --run make_wd --params filename:reuters.martin.tok,ws:8,ml:400
// ./wopr --run hapax_data --params hpx:1,filename:reuters.martin.tok.data
// ./wopr -r make_wd,hapax_data -p ws:7,filename:retrs.mrtn.tok,hpx:4,ml:1000
// ./wopr --run script -p filename:"etc/script",hpx:1
// ---------------------------------------------------------------------------

/*
  Anders.

  in script:
    set input file
    set filetype
    specify transformations.

    eg:
    input: text.txt (parameter)
    type: train (params)
    (parameters: lines:100, ws:7, hpx:3)
    transform: cut/lines:100, window_data/ws:7, hapax_data/hpx:3
*/

/*
  This file could have a number of functions which can be
  chained together maybe, like small building blocks
  to set up experiments.
  Like "make_test_test( params )" &c.
  These could be specified in config like:
    make_data, run_exp, &c.
  Make function factory, let all take same arguments (Config*),
  call one after the other?

  make data, filter1, make more data, run exp.

  take/define datasets etc. in Config? read a script?

  What do we want?
  1)scripts to make data
  2)run Timbl
  3)eval results

  Parse output of started programs, recognise KV pairs, store them.
  e.g. prog1 outputs "foo=bar", and foo is usable in prog2. useful
  to get Timbl output in this program?
  We need exec/fork as in PETeR...
*/

// ---- Experimental

int rrand(Logfile& l, Config& c ) {
  std::string filename = "/dev/video0";

  std::ifstream file_in( filename.c_str() );
  if ( ! file_in ) {
    l.log( "ERROR: cannot open file." );
    return -1;
  }
  std::ofstream file_out( "out.bin", std::ios::out );
  if ( ! file_out ) {
    l.log( "ERROR: cannot write file." );
    return -1;
  }
  std::string a_line;

  char* buffer;
  int length = 1024 * 8;
  buffer = new char[length];

  // read data as a block:
  //
  for ( int i = 0; i < 10; i++ ) {
    file_in.read( buffer, length );
    l.log( to_str( (int)file_in.gcount() ));
    file_out.write( buffer, length );
  }

  file_out.close();
  file_in.close();
  return 0;
}

// ---- End Experimental

#ifdef TIMBL
int timbl( Logfile& l, Config& c ) {
  l.log( "timbl");
  const std::string& timbl =  c.get_value("timbl");
  l.inc_prefix();
  l.log( "timbl:     "+timbl );
  l.log( "trainfile: "+c.get_value( "trainfile" ) );
  l.log( "testfile:  "+c.get_value( "testfile" ) );
  l.dec_prefix();

  //"-a IB2 +vF+DI+DB" ->  -timbl -- "-a IB2 +vF+DI+DB"
  Timbl::TimblAPI *My_Experiment = new Timbl::TimblAPI( timbl );
  My_Experiment->Learn( c.get_value( "trainfile" )); 

  const std::string& test_filename = c.get_value( "testfile" );
  if ( test_filename != "" ) {
    const std::string testout_filename = test_filename + ".out";
    My_Experiment->Test( test_filename, testout_filename );
  }
  delete My_Experiment;
  return 0;
}
#else
int timbl( Logfile& l, Config& c ) {
  l.log( "Timbl support not built in." );
  return -1;
}
#endif

#ifdef TIMBL
int make_ibase( Logfile& l, Config& c ) {
  l.log( "make_ibase");
  const std::string& timbl =  c.get_value("timbl");
  const std::string& filename = c.get_value( "filename" );
  const std::string& ibase_filename = filename + ".ibase";

  if ( file_exists(l, c, ibase_filename) ) {
    l.log( "IBASE exists, not overwriting." );
    c.add_kv( "ibasefile", ibase_filename );
    l.log( "SET ibasefile to "+ibase_filename );
    return 0;
  }

  l.inc_prefix();
  l.log( "timbl:     "+timbl );
  l.log( "filename:  "+c.get_value( "filename" ) );
  l.log( "ibasefile: "+ibase_filename );
  l.dec_prefix();

  Timbl::TimblAPI *My_Experiment = new Timbl::TimblAPI( timbl );
  My_Experiment->Learn( filename ); //c.get_value( "trainfile" )); 
  My_Experiment->WriteInstanceBase( ibase_filename );

  delete My_Experiment;

  c.add_kv( "ibasefile", ibase_filename );
  l.log( "SET ibasefile to "+ibase_filename );

  return 0;
}
#else
int make_ibase( Logfile& l, Config& c ) {
  l.log( "Timbl support not built in." );
  return -1;
}
#endif

int run_external( Logfile* l, Config *c ) {
  char line[1024];
  //system( (c->get_value( "external" )).c_str() );
  FILE *fp = popen( (c->get_value( "external" )).c_str(), "r" );
  while (fgets(line, 1024, fp) != NULL) {
    l->log( line );
  }
  return 0;
}

int dummy( Logfile* l, Config *c ) {
  l->log( "ERROR, dummy function." );
  return -1;
}

int count_lines( Logfile *l, Config *c ) {
  const std::string& filename = c->get_value( "filename" );
  l->log( "count_lines(filename/"+filename+")" );

  std::ifstream file( filename.c_str() );
  if ( ! file ) {
    l->log( "ERROR: cannot load file." );
    return -1;
  }

  std::string a_line;
  unsigned long lcount = 0;
  while( std::getline( file, a_line )) {
    ++lcount;
  }
  l->log( "lcount = " + to_str( lcount ));
  return 0;
}


int dump_kv(Logfile *l, Config *c)  {
  c->dump_kv();
  return 0;
}

int clear_kv(Logfile *l, Config *c)  {
  c->clear_kv();
  return 0;
}

int script(Logfile& l, Config& c)  {
  const std::string& filename = c.get_value( "script_filename" );
  l.log( "script(script_filename/"+filename+")" );

  std::ifstream file( filename.c_str() );
  if ( ! file ) {
    l.log( "ERROR: cannot load file." );
    return -1;
  }
  
  std::string a_line;
  while( std::getline( file, a_line )) {
    if ( a_line.length() == 0 ) {
      continue;
    }
    std::vector<std::string>cmds;
    std::vector<std::string>kv_pairs;

    int pos = a_line.find( ':', 0 );
    if ( pos != std::string::npos ) {
      std::string lhs = trim(a_line.substr( 0, pos ));
      std::string rhs = trim(a_line.substr( pos+1 ));

      if ( lhs == "run" ) {
	//l.log(rhs);
	int(*pt2Func2)(Logfile&, Config&) = get_function( rhs );
	int res = pt2Func2(l, c);
	//l.log( "Result = "+to_str(res) );// should abort if != 0
	if ( res != 0 ) {
	  return res;
	}
      }
      if ( lhs == "params" ) {
	Tokenize( rhs, kv_pairs, ',' );
	for ( int j = 0; j < kv_pairs.size(); j++ ) {
	  c.process_line( kv_pairs.at(j), true ); // Can override!
	  l.log( "Parameter: " + kv_pairs.at(j) );
	}
	kv_pairs.clear();
      }
      if ( lhs == "msg" ) {
	l.log( rhs );
      }
      // SET: optiones:
      // set: filename:output01
      // set: oldname:$filename
      //              ^ take value of parameter instead of string.
      //
      if ( lhs == "set" ) { 
	// set foo = foo + "t" ? (or add foo "t")
	Tokenize( rhs, kv_pairs, ',' );
	std::string kv = kv_pairs.at(0);
	int pos = kv.find( ':', 0 );
	if ( pos != std::string::npos ) {
	  std::string lhs = trim(kv.substr( 0, pos ));
	  std::string rhs = trim(kv.substr( pos+1 ));
	  if ( rhs.substr(0, 1) == "$" ) {
	    rhs = c.get_value( rhs.substr(1, rhs.length()-1) );
	  }
	  c.add_kv( lhs, rhs );
	  l.log( "SET "+lhs+" to "+rhs );
	}
      }
    }
  }
  
  return 0;
}

typedef int(*pt2Func)(Logfile*, Config*);
pt2Func fun_factory( const std::string& a_fname ) {
  if ( a_fname == "count_lines" ) {
    return &count_lines;
  } else if ( a_fname == "dump_kv" ) {
    return &dump_kv;
  } else if ( a_fname == "clear_kv" ) {
    return &clear_kv;
  }
  return &dummy;
}

// NEW

int tst(Logfile& l, Config& c)  {
  return 1;
}

typedef int(*pt2Func2)(Logfile&, Config&);
pt2Func2 get_function( const std::string& a_fname ) {
  if ( a_fname == "cut" ) {
    return &cut;
  } else if ( a_fname == "timbl" ) {
    return &timbl;
  } else if ( a_fname == "flatten" ) {
    return &flatten;
  } else if ( a_fname == "lowercase" ) {
    return &lowercase;
  } else if ( a_fname == "make_ibase" ) {
    return &make_ibase;
  } else if ( a_fname == "window" ) {
    return &window;
  } else if ( a_fname == "window_s" ) {
    return &window_s;
  } else if ( a_fname == "ngram" ) {
    return &ngram;
  } else if ( a_fname == "ngram_s" ) {
    return &ngram_s;
  } else if ( a_fname == "prepare" ) {
    return &prepare;
  } else if ( a_fname == "arpa" ) {
    return &arpa;
  } else if ( a_fname == "window_line" ) {
    return &window_line;
  } else if ( a_fname == "window_lr" ) {
    return &window_lr;
  } else if ( a_fname == "unk_pred" ) {
    return &unk_pred;
  } else if ( a_fname == "timbl_test" ) {
    return &timbl_test;
  } else if ( a_fname == "script" ) {
    return &script;
  } else if ( a_fname == "lexicon" ) {
    return &lexicon;
  } else if ( a_fname == "hapax" ) {
    return &hapax;
  } else if ( a_fname == "trainfile" ) {
    return &trainfile;
  } else if ( a_fname == "testfile" ) {
    return &testfile;
  } else if ( a_fname == "window_usenet" ) {
    return &window_usenet;
  } else if ( a_fname == "server" ) {
    return &server;
  } else if ( a_fname == "test" ) {
    return &test;
  } else if ( a_fname == "smooth" ) {
    return &smooth_old;
  } else if ( a_fname == "server2" ) {
    return &server2;
  } else if ( a_fname == "read_a3" ) {
    return &read_a3;
  } else if ( a_fname == "tag" ) {
    return &tag;
  } else if ( a_fname == "rrand" ) {
    return &rrand;
  } else if ( a_fname == "pplx" ) {
    return &pplx;
  } else if ( a_fname == "pplxs" ) {
    return &pplx_simple;
  } else if ( a_fname == "multi" ) {
    return &multi;
  } else if ( a_fname == "correct" ) {
    return &correct;
  } else if ( a_fname == "generate" ) {
    return &generate;
  } else if ( a_fname == "generate_server" ) {
    return &generate_server;
  } else if ( a_fname == "bfl" ) { // bounds_from_lex in lcontext.cc
    return &bounds_from_lex;
  } else if ( a_fname == "bfc" ) { // bounds_from_cnt in lcontext.cc
    return &bounds_from_cnt;
  } else if ( a_fname == "lcontext" ) { // from lcontext.cc
    return &lcontext;
  }
  return &tst;
}

// parameter: filename
//            lines
//            (skip/start, etc)
//
int cut(Logfile& l, Config& c) {
  l.log( "cut" );
  const std::string& filename = c.get_value( "filename" );
  unsigned long lines = stol( c.get_value( "lines", "0" ));
  std::string output_filename = filename + ".cl";
  if ( lines > 0 ) {
    output_filename = output_filename + to_str(lines);
  }
  l.inc_prefix();
  l.log( "filename: "+filename );
  l.log( "lines:    " + to_str(lines) );
  l.log( "OUTPUT:   "+output_filename );
  l.dec_prefix();

  std::ifstream file_in( filename.c_str() );
  if ( ! file_in ) {
    l.log( "ERROR: cannot load file." );
    return -1;
  }
  std::ofstream file_out( output_filename.c_str(), std::ios::out );
  if ( ! file_out ) {
    l.log( "ERROR: cannot write file." );
    return -1;
  }
  
  std::string a_line;
  unsigned long lcount = 0;
  while( std::getline( file_in, a_line )) {
    file_out << a_line << std::endl;
    ++lcount;
    if ( lcount == lines ) {
      break;
    }
  }
  file_out.close();
  file_in.close();

  // Should we check if we got as many lines as we requested?

  c.add_kv( "filename", output_filename );
  l.log( "SET filename to "+output_filename );
  return 0;
}

// parameter: filename, lines (lines can be cut also...)
// Create a single columnfile with the sentences in filename.
//
int flatten(Logfile& l, Config& c) {
  l.log( "flatten" );
  const std::string& filename = c.get_value( "filename" );
  unsigned long lines = stol( c.get_value( "lines", "0" ));
  std::string output_filename = filename + ".fl";
  if ( lines > 0 ) {
    output_filename = output_filename + to_str(lines);
  }
  l.inc_prefix();
  l.log( "filename: "+filename );
  l.log( "OUTPUT:   "+output_filename );
  l.dec_prefix();

  std::ifstream file_in( filename.c_str() );
  if ( ! file_in ) {
    l.log( "ERROR: cannot load file." );
    return -1;
  }
  std::ofstream file_out( output_filename.c_str(), std::ios::out );
  if ( ! file_out ) {
    l.log( "ERROR: cannot write file." );
    return -1;
  }
  
  std::string a_word;
  unsigned long lcount = 0;
  while( file_in >> a_word ) {
    file_out << a_word << std::endl;
    ++lcount;
    if ( lcount == lines ) {
      break;
    }
  }
  file_out.close();
  file_in.close();
  
  l.log( "Count: " + to_str(lcount) );

  // Should we check if we got as many lines as we requested?
  
  c.add_kv( "filename", output_filename );
  l.log( "SET filename to "+output_filename );
  return 0;
}

// parameter: filename, lines (lines can be cut also...)
// Create a single columnfile with the sentences in filename.
//
int lowercase(Logfile& l, Config& c) {
  l.log( "lowercase" );
  const std::string& filename = c.get_value( "filename" );
  std::string output_filename = filename + ".lc";
  l.inc_prefix();
  l.log( "filename: "+filename );
  l.log( "OUTPUT:   "+output_filename );
  l.dec_prefix();

  std::ifstream file_in( filename.c_str() );
  if ( ! file_in ) {
    l.log( "ERROR: cannot load file." );
    return -1;
  }
  std::ofstream file_out( output_filename.c_str(), std::ios::out );
  if ( ! file_out ) {
    l.log( "ERROR: cannot write file." );
    return -1;
  }
  
  std::string a_line;
  while ( getline( file_in, a_line )) {
    std::transform(a_line.begin(),a_line.end(),a_line.begin(),tolower); 
    file_out << a_line << std::endl;
  }
  file_out.close();
  file_in.close();
  
  // Should we check if we got as many lines as we requested?
  
  c.add_kv( "filename", output_filename );
  l.log( "SET filename to "+output_filename );
  return 0;
}

// parameter: filename
//            ws
//
// Sentence mode?
//
int window( Logfile& l, Config& c ) {
  l.log( "window" );
  const std::string& filename        = c.get_value( "filename" );
  int                ws              = stoi( c.get_value( "ws", "3" ));
  bool               to_lower        = stoi( c.get_value( "lc", "0" )) == 1;
  std::string        output_filename = filename + ".ws" + to_str(ws);
  l.inc_prefix();
  l.log( "filename:  "+filename );
  l.log( "ws:        "+to_str(ws) );
  l.log( "lowercase: "+to_str(to_lower) );
  l.log( "OUTPUT:    "+output_filename );
  l.dec_prefix();

  std::ifstream file_in( filename.c_str() );
  if ( ! file_in ) {
    l.log( "ERROR: cannot load file." );
    return -1;
  }
  std::ofstream file_out( output_filename.c_str(), std::ios::out );
  if ( ! file_out ) {
    l.log( "ERROR: cannot write file." );
    return -1;
  }

  // initialise with _
  // loop:
  //   read word,
  //   output window
  //   output word (the class)
  //   shift whole window down, put class at last
  // copy( v2.begin( )+4, v2.begin( ) + 7, v2.begin( ) + 2 );
  //       first          last             begin pos
  // until nomorewords.
  std::string               a_word;
  std::vector<std::string>  window(ws+1, "_");
  std::map<std::string,int> count;
  std::vector<std::string>::iterator vi;
  std::ostream_iterator<std::string> output( file_out, " " );

  while( file_in >> a_word ) {

    if ( to_lower ) {
      std::transform(a_word.begin(),a_word.end(),a_word.begin(),tolower); 
    }

    std::copy( window.begin(), window.end()-1, output );
    file_out << a_word << std::endl;
    
    window.at(ws) = a_word;
    copy( window.begin()+1, window.end(), window.begin() );
  }
  file_out.close();
  file_in.close();

  c.add_kv( "filename", output_filename );
  l.log( "SET filename to "+output_filename );
  return 0;
}

// parameter: filename
//            ws
//
// Sentence mode.
//
int window_s( Logfile& l, Config& c ) {
  l.log( "window_s" );
  const std::string& filename        = c.get_value( "filename" );
  int                ws              = stoi( c.get_value( "ws", "3" ));
  bool               to_lower        = stoi( c.get_value( "lc", "0" )) == 1;
  std::string        output_filename = filename + ".ws" + to_str(ws);
  std::string        pre_s           = c.get_value( "pre_s", "" );
  std::string        suf_s           = c.get_value( "suf_s", "" );
  int                skip            = 0;

  if ( file_exists( l, c, output_filename ) ) {
    l.log( "DATASET exists, not overwriting." );
    c.add_kv( "filename", output_filename );
    l.log( "SET filename to "+output_filename );
    return 0;
  }

  // If we specify a sentence begin marker, we skip the "_ _ .." patterns with
  // skip (duh).
  //
  if ( pre_s != "" ) {
    pre_s = pre_s + " ";
    skip = ws;
  }
  if ( suf_s != "" ) {
    suf_s = " " + suf_s;
  }
  l.inc_prefix();
  l.log( "filename:  "+filename );
  l.log( "ws:        "+to_str(ws) );
  l.log( "lowercase: "+to_str(to_lower) );
  l.log( "pre_s    : "+pre_s );
  l.log( "suf_s    : "+suf_s );
  l.log( "OUTPUT:    "+output_filename );
  l.dec_prefix();

  std::ifstream file_in( filename.c_str() );
  if ( ! file_in ) {
    l.log( "ERROR: cannot load file." );
    return -1;
  }
  std::ofstream file_out( output_filename.c_str(), std::ios::out );
  if ( ! file_out ) {
    l.log( "ERROR: cannot write file." );
    return -1;
  }
  
  std::string a_line;
  std::vector<std::string> results;
  std::vector<std::string> targets;
  std::vector<std::string>::iterator ri;

  while( std::getline( file_in, a_line )) {

    if ( to_lower ) {
      std::transform(a_line.begin(),a_line.end(),a_line.begin(),tolower); 
    }

    //std::string t = a_line;

    a_line = pre_s + a_line + suf_s;

    window( a_line, a_line, ws, 0, false, results );
    if ( (skip == 0) || (results.size() >= ws) ) {
      for ( ri = results.begin()+skip; ri != results.end(); ri++ ) {
	std::string cl = *ri;
	file_out << cl << std::endl;
	//l.log( cl );
      }
      results.clear();
    } else {
      l.log( "SKIP: " + a_line );
    }
  }
  file_out.close();
  file_in.close();

  c.add_kv( "filename", output_filename );
  l.log( "SET filename to "+output_filename );
  return 0;
}

int ngram( Logfile& l, Config& c ) {
  l.log( "ngram" );
  const std::string& filename        = c.get_value( "filename" );
  int                ws              = stoi( c.get_value( "ws", "3" ));
  std::string        output_filename = filename + ".ng" + to_str(ws);
  l.inc_prefix();
  l.log( "filename:  "+filename );
  l.log( "ws:        "+to_str(ws) );
  l.log( "OUTPUT:    "+output_filename );
  l.dec_prefix();

  std::ifstream file_in( filename.c_str() );
  if ( ! file_in ) {
    l.log( "ERROR: cannot load file." );
    return -1;
  }
  std::ofstream file_out( output_filename.c_str(), std::ios::out );
  if ( ! file_out ) {
    l.log( "ERROR: cannot write file." );
    return -1;
  }

  // initialise with _
  // loop:
  //   read word,
  //   output window
  //   output word (the class)
  //   shift whole window down,
  // copy( v2.begin( )+4, v2.begin( ) + 7, v2.begin( ) + 2 );
  //       first          last             begin pos
  // until nomorewords.
  std::string               a_word;
  std::vector<std::string>  window(ws, "_");
  std::map<std::string,int> count;
  std::vector<std::string>::iterator vi;
  std::ostream_iterator<std::string> output( file_out, " " );

  for ( int i = 0; i < ws; i++ ) {
    file_in >> a_word;
     window.at(i) = a_word;    
  }

  while( file_in >> a_word ) {

    std::copy( window.begin(), window.end(), output );
    file_out << std::endl;
    
    copy( window.begin()+1, window.end(), window.begin() );
    window.at(ws-1) = a_word;
  }

  std::copy( window.begin(), window.end(), output );
  file_out << std::endl;
  
  file_out.close();
  file_in.close();

  c.add_kv( "filename", output_filename );
  l.log( "SET filename to "+output_filename );
  return 0;
}

// Sentence/line based ngram function.
//
int ngram_s( Logfile& l, Config& c ) {
  l.log( "ngram_s" );
  const std::string& filename        = c.get_value( "filename" );
  int                ws              = stoi( c.get_value( "ws", "3" ));
  std::string        output_filename = filename + ".ng" + to_str(ws);
  bool               to_lower        = stoi( c.get_value( "lc", "0" )) == 1;
  l.inc_prefix();
  l.log( "filename:  "+filename );
  l.log( "ws:        "+to_str(ws) );
  l.log( "OUTPUT:    "+output_filename );
  l.dec_prefix();

  std::ifstream file_in( filename.c_str() );
  if ( ! file_in ) {
    l.log( "ERROR: cannot load file." );
    return -1;
  }
  std::ofstream file_out( output_filename.c_str(), std::ios::out );
  if ( ! file_out ) {
    l.log( "ERROR: cannot write file." );
    return -1;
  }

  std::string a_line;
  std::vector<std::string> results;
  std::vector<std::string>::iterator ri;

  while( std::getline( file_in, a_line )) {

    if ( to_lower ) {
      std::transform(a_line.begin(),a_line.end(),a_line.begin(),tolower); 
    }

    ngram_line( a_line, ws, results );
    for ( ri = results.begin(); ri != results.end(); ri++ ) {
      std::string cl = *ri;
      file_out << cl << std::endl;
    }
    results.clear();
  }
  file_out.close();
  file_in.close();

  c.add_kv( "filename", output_filename );
  l.log( "SET filename to "+output_filename );
  return 0;
}

// Take a line-based file write it word-based with the sentences
// wrapped in <s> and </s>.
// This way we can feed it in the windowing/ngram functions.
// Actually, the word based is not necessary...
//
int prepare( Logfile& l, Config& c ) {
  l.log( "prepare" );
  const std::string& filename        = c.get_value( "filename" );
  std::string        pre_s           = c.get_value( "pre_s", "<s>" );
  std::string        suf_s           = c.get_value( "suf_s", "</s>" );
  std::string        output_filename = filename + ".p";

  l.inc_prefix();
  l.log( "filename: "+filename );
  l.log( "pre_s   : "+pre_s );
  l.log( "suf_s   : "+suf_s );
  l.log( "OUTPUT  : "+output_filename );
  l.dec_prefix();

  std::ifstream file_in( filename.c_str() );
  if ( ! file_in ) {
    l.log( "ERROR: cannot load file." );
    return -1;
  }

  std::ofstream file_out( output_filename.c_str(), std::ios::out );
  if ( ! file_out ) {
    l.log( "ERROR: cannot write file." );
    return -1;
  }

  std::string a_line, a_word;
  std::vector<std::string> words;
  std::vector<std::string>::iterator wi;
  while( std::getline( file_in, a_line )) {
    //
    // How to split the line?
    //
    file_out << pre_s << " "; //std::endl;

    /*
    Tokenize( a_line, words, ' ' );
    for ( wi = words.begin(); wi != words.end(); wi++ )  {
      a_word = *wi;
      if ( a_word != "" ) {
	file_out << a_word << std::endl;
      }
    }
    words.clear();
    */
    file_out << a_line << " " << suf_s << std::endl;
  } 
  file_out.close();
  file_in.close();

  return 0;
}

// Read an ngn file, create arpa format
//
int arpa( Logfile& l, Config& c ) {
  l.log( "arpa" );
  const std::string& filename        = c.get_value( "filename" );
  std::string        output_filename = filename + ".arpa";
  const int          precision       = stoi( c.get_value( "precision", "6" ));
  int                ws              = stoi( c.get_value( "ws", "3" ));

  l.inc_prefix();
  l.log( "filename:  "+filename );
  l.log( "ws:        "+to_str(ws) );
  l.log( "OUTPUT:    "+output_filename );
  l.dec_prefix();

  std::ifstream file_in( filename.c_str() );
  if ( ! file_in ) {
    l.log( "ERROR: cannot load file." );
    return -1;
  }

  std::map<std::string,int> count;
  std::string a_line;
  int total_count = 0;
  while( std::getline( file_in, a_line )) {
    count[ a_line ]++;
    ++total_count;
  } 
  file_in.close();

  /*
  std::map<int,int> ffreqs; // frequency of frequencies
  int max_freq = 0;
  for( std::map<std::string,int>::iterator iter = count.begin(); iter != count.end(); iter++ ) {
    int ngfreq = (*iter).second;
    if ( ngfreq > max_freq ) {
      max_freq = ngfreq;
    }
    ffreqs[ngfreq]++;
  }
  */

  std::ofstream file_out( output_filename.c_str(), std::ios::out );
  if ( ! file_out ) {
    l.log( "ERROR: cannot write file." );
    return -1;
  }

  file_out << "\\data\\\n";
  file_out << "ngram " << ws << "=" << count.size() << "\n";
  file_out << "\\" << ws << "-grams:\n";

  for( std::map<std::string,int>::iterator iter = count.begin(); iter != count.end(); iter++ ) {
    int ngfreq = (*iter).second;
    double cprob = (double)ngfreq / (double)total_count;
    //file_out << to_str(cprob, precision) << " " << (*iter).first << "\n";
    file_out << log(cprob) << " " << (*iter).first << "\n";
  }

  file_out << "\\end\\\n";
  file_out.close();

  c.add_kv( "filename", output_filename );
  l.log( "SET filename to "+output_filename );
  return 0;
}

/*./wopr --run window_line -p classify:"Pork bellies also closed sharply lower because more hogs translates to more pork",ibasefile:tekst.txt.l100000.ws7.ibase

./wopr --run window_line -p classify:"dit is een zin and sentence .",ibasefile:tekst.txt.l1000.ws7.ibase,timbl:"-a1 +D"
 */
#ifdef TIMBL
int window_line(Logfile& l, Config& c) {
  // initialise with _
  // loop:
  //   read word,
  //   output window
  //   output word (the class)
  //   shift whole window down, put class at last
  // copy( v2.begin( )+4, v2.begin( ) + 7, v2.begin( ) + 2 );
  //       first          last             begin pos
  // until nomorewords.
  //
  // Have target as argument?
  //
  int ws = stoi( c.get_value( "ws", "7" ));
  std::string              a_word;
  std::vector<std::string> words; // input.
  std::vector<std::string> res;
  std::vector<std::string> word_stats; // perplx. per word.
  std::vector<std::string> window(ws+1, "_"); //context + target
  std::vector<std::string>::iterator wi;
  std::ostream_iterator<std::string> output( std::cout, " " );
  const std::string& a_line = c.get_value( "classify", "error" );
  const std::string& timbl  =  c.get_value("timbl");
  Tokenize( a_line, words, ' ' );

  std::string classify_line;
  int correct = 0;
  int possible = 0;
  int sentence_length = 0;
  try {
    Timbl::TimblAPI *My_Experiment = new Timbl::TimblAPI( timbl );
    (void)My_Experiment->GetInstanceBase( c.get_value( "ibasefile" ));
    std::string distrib;
    std::vector<std::string> distribution;
    std::string result;
    double distance;
    double total_prplx = 0.0;
    const Timbl::ValueDistribution *vd;
    const Timbl::TargetValue *tv;

    //window.at(ws-1) = words.at(0); // shift in the first word
    //                      +1 in the above case.
    for ( wi = words.begin()+0; wi != words.end(); wi++ )  {
      ++sentence_length;
      a_word = *wi;
      std::copy( window.begin(), window.end()-1, std::inserter(res, res.end()));
      res.push_back(a_word); // target
      for ( int i = 0; i < ws; i++ ) {
	classify_line = classify_line + window.at(i)+" ";
      }
      classify_line = classify_line + a_word; // target
      window.at(ws) = a_word;
      // Classify with the TimblAPI
      My_Experiment->Classify( classify_line, result, distrib, distance );
      //tv = My_Experiment->Classify( classify_line, vd );

      std::cout << "[" << classify_line << "] = " << result 
		<< "/" << distance << " "
	//<< distrib << " "
		<< std::endl;
      classify_line = "";
      if ( result == a_word ) {
	l.log( "Correcto" );
	++correct;
      }

      // Begin experimental
      // { is 2.00000, misschien 1.00000 }
      //
      Tokenize( distrib, distribution, ' ' ); // { TOK0 num, TOK1 num }
      //
      // nu hebben we paren? Eerste { overslaan.
      //
      bool is_class = true;
      int  sum      = 0;
      int  d_size   = 0;
      int  target_f = 0;
      for ( int i = 1; i < distribution.size(); i++ ) {
	std::string token = distribution.at(i);
	if ( (token == "{") || (token == "}")) {
	  continue;
	}
	if ( is_class ) {
	  ++d_size;
	  if ( token == a_word ) {
	    token = "In distro: "+token; //(would have been) the correct prediction.
	    l.log( token + " / " + distribution.at(i+1) );
	    target_f = stoi(distribution.at(i+1));
	    ++possible;
	  }
	  //l.log_begin( token );
	} else {
	  token = token.substr(0, token.length()-1);
	  sum += stoi( token );
	  //l.log_end( " - "+token );
	}
	is_class = ( ! is_class );
      }
      if ( d_size < 20 ) {
	l.log( distrib );
      }
      //
      // sum is the sum of frequencies in distro.
      // d_size is the number of items in distro.
      //
      l.log( "sum="+to_str(sum)+", d_size="+to_str(d_size));
      double prplx = 1.0;
      if ( sum != 0 ) {
	prplx = (double)target_f / (double)sum;
      }
      word_stats.push_back( "prplx("+a_word+"/"+result+") = " + to_str( 1 - prplx ));
      l.log( "prplx = " + to_str( 1 - prplx ));
      total_prplx += prplx;
      distribution.clear();
      d_size = 0;
      //
      // End experimental
      /*
      if ( vd ) {
	int count = 0;
	Timbl::ValueDistribution::dist_iterator it=vd->begin();      
	while ( it != vd->end() ){
	  //Timbl::Vfield *foo = it->second;
	  //const Timbl::TargetValue *bar = foo->Value();
	  std::string quux = it->second->Value()->Name(); //bar->Name();
	  if ( quux == a_word ) {
	    std::cout << "Found a_word.\n";
	    std::cout << it->second->Value() << ": "
		      << it->second->Weight() << std::endl;
	  }
	  ++it;
	  const Timbl::TargetValue *tarv = it->second->Value();
	  
	  if ( --count == 0 ) {
	    it = vd->end();
	  }
	}
      }
      */
      copy( window.begin()+1, window.end(), window.begin() );
    }
    //
    // Normalize to sentence length. Subtract from 1 (0 means
    // all wrong, totally surprised).
    //
    total_prplx = total_prplx / sentence_length;
    l.log( "correct     = " + to_str(correct)+"/"+to_str(sentence_length));
    l.log( "possible    = " + to_str(possible));
    l.log( "total_prplx = " + to_str( 1 - total_prplx ));

    for ( int i = 0; i < sentence_length; i++ ) {
      l.log( word_stats.at(i) );
    }
  }
  catch ( const std::exception& e ) {
    l.log( "ERROR: exception caught." );
    return -1;
  }

  return 0;
}
#else
int window_line( Logfile& l, Config& c ) {
  l.log( "No TIMBL support." );
  return -1;
}
#endif

// NB: window(c,l) exists as well.
//
// Q: large data set "as one line" ?
//
// Supply targets? It should be a function...
// Now it is a string which will be tokenized. Differentiate between
// null en "" ?
//
// Have a seperate n-gram function?
//
// Backoff, indicator? It would shorten the instance, leave the target
// the same, so it generates the kku patterns.
// a b c -> d
// a b ->d with a '1 backoff'
//
int window( std::string a_line, std::string target_str, 
	    int lc, int rc, bool var, 
	    std::vector<std::string>& res ) {

  std::vector<std::string> words; //(10000000,"foo");
  Tokenize( a_line, words );

  std::vector<std::string> targets;
  if ( target_str == "" ) {
    targets = std::vector<std::string>( words.size(), "" ); // or nothing?
  } else {
    Tokenize( target_str, targets );
  }

  //std::vector<std::string> V; (V -> words)
  //copy(istream_iterator<std::string>(cin), istream_iterator<std::string>(),
  //     back_inserter(V));   

  std::vector<std::string> full(lc+rc, "_"); // initialise a full window.
  //full[lc+rc-1] = "<S>";

  //
  // ...and insert the words at the position after the left context.
  //                                     can we do this from a file?
  //                                               |
  std::copy( words.begin(), words.end(), std::inserter(full, full.begin()+lc));

  std::vector<std::string>::iterator si;
  std::vector<std::string>::iterator fi;
  std::vector<std::string>::iterator ti = targets.begin();
  std::string windowed_line = "";
  si = full.begin()+lc; // first word of sentence.
  int factor = 0; // lc for variable length instances.
  if ( var == true ) {
    factor = lc;
  }
  for ( int i = 0; i < words.size(); i++ ) {
    //mark/target is at full(i+lc)
    
    for ( fi = si-lc+factor; fi != si+1+rc; fi++ ) { // context around si
      if ( fi != si ) {
	//spacer = (*fi == "") ? "" : " ";
	windowed_line = windowed_line + *fi + " ";
      }/* else { // the target, but we don't show that here.
	windowed_line = windowed_line + "(T) ";
	}*/
    }
    windowed_line = windowed_line + *ti; // target. function to make target?
    res.push_back( windowed_line );
    windowed_line.clear();
    si++;
    ti++;
    if ( factor > 0 ) {
      --factor;
    }
  }

  return 0;
}

// With 'backoff'. See comments above. Right context doesn't make sense here.
//
int window( std::string a_line, std::string target_str, 
	    int lc, int rc, int bo,
	    std::vector<std::string>& res ) {

  std::vector<std::string> words;
  Tokenize( a_line, words );

  std::vector<std::string> targets;
  if ( target_str == "" ) {
    targets = std::vector<std::string>( words.size(), "" ); // or nothing?
  } else {
    Tokenize( target_str, targets );
  }

  std::vector<std::string> full(lc, "_"); // initialise a full window.

  //
  // ...and insert the words at the position after the left context.
  //                                     can we do this from a file?
  //                                               |
  std::copy( words.begin(), words.end(), std::inserter(full, full.begin()+lc));

  std::vector<std::string>::iterator si;
  std::vector<std::string>::iterator fi;
  std::vector<std::string>::iterator ti = targets.begin();
  std::string windowed_line = "";
  si = full.begin()+lc; // first word of the instance
  int factor = lc; // lc for variable length instances.
  for ( int i = 0; i < words.size(); i++ ) {
    for ( fi = si-lc; fi != si-bo; fi++ ) { // context around si
      if ( fi != si ) {
	windowed_line = windowed_line + *fi + " ";
      }
    }
    if ( factor > 0 ) {
      --factor;
    } else {
      windowed_line = windowed_line + *ti; // target. Function to make target?
      res.push_back( windowed_line );
    }
    windowed_line.clear();
    si++;
    ti++;
  }

  return 0;
}

// Full windowing on a file, left/right contexts
//
int window_lr( Logfile& l, Config& c ) {
  l.log( "window_lr" );
  const std::string& filename        = c.get_value( "filename" );
  int                lc              = stoi( c.get_value( "lc", "3" ));
  int                rc              = stoi( c.get_value( "rc", "3" ));
  std::string        output_filename = filename + ".l" + to_str(lc) + "r" + to_str(rc);
  l.inc_prefix();
  l.log( "filename:  "+filename );
  l.log( "lc:        "+to_str(lc) );
  l.log( "rc:        "+to_str(rc) );
  l.log( "OUTPUT:    "+output_filename );
  l.dec_prefix();

  std::ifstream file_in( filename.c_str() );
  if ( ! file_in ) {
    l.log( "ERROR: cannot load file." );
    return -1;
  }
  std::ofstream file_out( output_filename.c_str(), std::ios::out );
  if ( ! file_out ) {
    l.log( "ERROR: cannot write file." );
    return -1;
  }

  std::string                        a_line;
  std::vector<std::string>           results;
  std::vector<std::string>::iterator ri;
  while( std::getline( file_in, a_line ) ) { 
    window( a_line, a_line, lc, rc, false, results );
    for ( ri = results.begin(); ri != results.end(); ri++ ) {
      file_out << *ri << "\n";
    }
    results.clear();
  }

  file_out.close();
  file_in.close();

  c.add_kv( "filename", output_filename );
  l.log( "SET filename to "+output_filename );
  return 0;
}

// Hapax one line only.
// Everything that is NOT in wfreqs is replaced by HAPAX.
// Only called from server2, maybe move it there.
// Unlike the hapax() function, the actual frequency is checked directly.
//
// This is called from server2 - do not use otherwise.
//
int hapax_line( const std::string& a_line, std::map<std::string,int> wfreqs,
		int hpx, int hpx_t, std::string& res ) {
  std::vector<std::string> words;
  Tokenize( a_line, words, ' ' );
  std::vector<std::string>::iterator wi;
  std::map<std::string, int>::iterator wfi;
  std::string hpx_sym = "<unk>"; //c.get_value("hpx_sym", "<unk>");

  //  replace_if with bind2nd( .. )
  
  /*
  wfreqs["_"]    = hpx+1;
  wfreqs["<s>"]  = hpx+1;
  wfreqs["</s>"] = hpx+1;
  */

  res.clear();
  std::string wrd;
  for ( int i = 0; i < words.size(); i++ ) {
    wrd = words.at( i );
    wfi = wfreqs.find( wrd );

    if ( wfi == wfreqs.end() ) { // not found in lexicon
      res = res + hpx_sym + " ";
    } else {
      //int f = (*wfi).second;
      //if ( f > hpx ) {
      res = res + wrd + " ";
	//} else {
	//res = res + hpx_sym + " ";
	//}
    }
  } 

  // remove last " "
  //
  res.resize(res.size()-1);

  return 0;
}

// The unknown words predictor, double-wopr, &c.
// Takes a file, checks each focus word(target), if it is unknown,
// write to file? test?
// Regexpsen for numbers, Entities.
//
#ifdef TIMBL
int unk_pred( Logfile& l, Config& c ) {
  l.log( "unk_pred" );
  const std::string& timbl     = c.get_value("timbl");
  const std::string& ibasefile = c.get_value("ibasefile");
  const std::string& testfile  = c.get_value("testfile"); // We'll make data
  const std::string& lexfile   = c.get_value("lexicon");
  std::string        output_filename = testfile + ".up";
  l.inc_prefix();
  l.log( "ibasefile: "+ibasefile );
  l.log( "testfile:  "+testfile );
  l.log( "lexicon:   "+lexfile );
  l.log( "timbl:     "+timbl );
  l.log( "OUTPUT:    "+output_filename );
  l.dec_prefix();

  // Load lexicon. NB: hapaxed lexicon is different? Or add HAPAX entry?
  //
  std::ifstream file_lexicon( lexfile.c_str() );
  if ( ! file_lexicon ) {
    l.log( "ERROR: cannot load lexicon file." );
    return -1;
  }
  // Read the lexicon with word frequencies.
  // We need a hash with frequence - countOfFrequency, ffreqs.
  //
  l.log( "Reading lexicon." );
  std::string a_word;
  int wfreq;
  unsigned long total_count = 0;
  std::map<std::string,int> wfreqs; // whole lexicon
  while( file_lexicon >> a_word >> wfreq ) {
    wfreqs[a_word] = wfreq;
    total_count += wfreq;
  }
  file_lexicon.close();
  l.log( "Read lexicon (total_count="+to_str(total_count)+")." );

  // Open test file.
  //
  std::ifstream file_in( testfile.c_str() );
  if ( ! file_in ) {
    l.log( "ERROR: cannot load test file." );
    return -1;
  }

  std::ofstream file_out( output_filename.c_str(), std::ios::out );
  if ( ! file_out ) {
    l.log( "ERROR: cannot write output file." );
    return -1;
  }

  // Set up Timbl.
  //
  std::string distrib;
  std::string a_line;
  std::vector<std::string> distribution;
  std::vector<std::string> words;
  std::string result;
  double distance;
  double total_prplx = 0.0;
  const Timbl::ValueDistribution *vd;
  const Timbl::TargetValue *tv;
  int unknown_count = 0;
  int skipped_num = 0;
  int skipped_ent = 0;

  try {
    Timbl::TimblAPI *My_Experiment = new Timbl::TimblAPI( timbl );
    (void)My_Experiment->GetInstanceBase( c.get_value( "ibasefile" ));

    std::vector<std::string> results;
    std::vector<std::string>::iterator ri;

    // Loop over the test file.
    // The test file is in format l-1,l-2,l-3,...
    //
    while( std::getline( file_in, a_line ) ) {  

      a_line = trim( a_line, " \n\r" );
      //std::cout << a_line << std::endl;

      std::string new_line; // The new one with unk words replaced.

      // Tokenize the line of data, check for unknowns.
      //
      Tokenize( a_line, words, ' ' );
      for ( int i = 0; i < words.size(); i++ ) {
	
	std::string word = words.at(i);

	// Check if it is known/unknown. Check for numbers and entities.
	// (Make these regexpen a parameter?) Before or after unknown check?
	// We need a regexp  library...
	//
	if ( (word[0] > 64) && (word[0] < 91) && (i > 0 ) ) {
	  ++skipped_ent;
	  new_line = new_line + word + " ";
	  continue;
	  std::cout << "ENTITY: " << word << std::endl;
	}
	if ( is_numeric( word ) ) {
	  ++skipped_num;
	  new_line = new_line + word + " ";
	  continue;
	  std::cout << "NUMERIC: " << word << std::endl;
	}
	if ( wfreqs.find( words.at(i) ) == wfreqs.end() ) { // not found 
	  //
	  // Unknown, we classify.
	  //
	  //std::cout << "'" << word << "' <unk>\n";
	  ++unknown_count;

	  // Create a pattern for this word. The pattern we want is
	  // at index i.
	  //
	  window( a_line, "", 2, 2, false, results ); // hardcoded (now) l2r2 format
	  std::string cl = results.at(i);      // line to classify
	  cl = cl + "TARGET";
	  //std::cout << cl << std::endl;
	  My_Experiment->Classify( cl, result, distrib, distance );
	  //std::cout << "CLAS=" << result << std::endl;
	  new_line = new_line + result + " ";
	  results.clear();
	} else {
	  new_line = new_line + word + " ";
	}
      }
      words.clear();
      results.clear();

      //std::cout << new_line << std::endl;
      //std::cout << std::endl;
      
      file_out << new_line << std::endl;

      /*
      // Now create a .ws3 data line for the new line.
      //
      window( new_line, a_line, ws, 0, results );
      if ( results.size() >= ws ) {
	for ( ri = results.begin(); ri != results.end(); ri++ ) {
	  std::string cl = *ri;
	  file_out << cl << std::endl;
	}
	results.clear();
      } else {
	l.log( "SKIP: " + new_line );
      }
      */

    } // getline
  } // try
  catch ( const std::exception& e ) {
    l.log( "ERROR: exception caught." );
    return -1;
  }

  file_out.close();
  file_in.close();

  l.log( "Unknowns: " + to_str(unknown_count));
  l.log( "Numeric : " + to_str( skipped_num ));
  l.log( "Entity  : " + to_str( skipped_ent ));

  return 0;
}
#else
int unk_pred( Logfile& l, Config& c ) {
  l.log( "Timbl support not built in." );
  return -1;
}
#endif

// Version with targets?
//
int ngram_line( std::string a_line, int n, std::vector<std::string>& res ) {

  std::vector<std::string> words;
  Tokenize( a_line, words, ' ' );

  // Or fill with empty context?
  //
  if ( words.size() < n ) {
    return 1;
  }

  std::vector<std::string>::iterator wi;
  std::vector<std::string>::iterator ngri;

  std::string w_line = "";
  wi = words.begin(); // first word of sentence.
  for ( int i = 0; i < words.size()-n+1; i++ ) {
    
    for ( ngri= wi; ngri < wi+n; ngri++ ) {
      w_line = w_line + *ngri + " "; // no out of bounds checking.
    }
    //std::cout << w_line << std::endl;
    res.push_back( w_line );

    w_line.clear();
    wi++;
  }

  return 0;
}

// If we add a .lex file, do the double-wopr? Hm, this does file based.
//
// ./wopr --run timbl_test -p ibasefile:tekst.txt.l100000.ws7.ibase,testfile:tekst.txt.l1000.ws7,timbl:-a1
//
#ifdef TIMBL
int timbl_test(Logfile& l, Config& c) {
  l.log( "timb_test" );
  const std::string& timbl     =  c.get_value("timbl");
  const std::string& ibasefile =  c.get_value("ibasefile");
  const std::string& testfile  =  c.get_value("testfile");
  l.inc_prefix();
  l.log( "ibasefile: "+ibasefile );
  l.log( "testfile:  "+testfile );
  l.dec_prefix();

  Timbl::TimblAPI *My_Experiment = new Timbl::TimblAPI( timbl );
  (void)My_Experiment->GetInstanceBase( ibasefile );

  const std::string testout_filename = testfile + ".out";
  My_Experiment->Test( testfile, testout_filename );
  delete My_Experiment;

  return 0;
}
#else
int timbl_test( Logfile& l, Config& c ) {
  l.log( "No TIMBL support." );
  return -1;
}
#endif

// input: filename
// writes: filename.lex, adds "lexicon" to kv.
//
int lexicon(Logfile& l, Config& c) {
  l.log( "lexicon" );
  const std::string& filename        = c.get_value( "filename" );
  std::string        output_filename = filename + ".lex";
  std::string        counts_filename = filename + ".cnt";
  std::string        anaval_filename = filename + ".av";
  std::string        mode            = c.get_value( "mode", "word" );
  bool               to_lower        = stoi( c.get_value( "lc", "0" )) == 1;
  l.inc_prefix();
  l.log( "filename: "+filename );
  l.log( "lowercase: "+to_str(to_lower) );
  l.log( "OUTPUT:   "+output_filename );
  l.log( "OUTPUT:   "+counts_filename );
  l.log( "OUTPUT:   "+anaval_filename );
  l.dec_prefix();

  if ( file_exists(l,c,output_filename) && file_exists(l,c,counts_filename) ) {
    l.log( "LEXICON and COUNTSFILE exists, not overwriting." );
    c.add_kv( "lexicon", output_filename );
    l.log( "SET lexicon to "+output_filename );
    return 0;
  }

  std::ifstream file_in( filename.c_str() );
  if ( ! file_in ) {
    l.log( "ERROR: cannot load file." );
    return -1;
  }
  
  std::string a_word;
  std::map<std::string,int> count;
  if ( mode == "word" ) {
    while( file_in >> a_word ) {  //word based
      
      if ( to_lower ) {
	std::transform(a_word.begin(),a_word.end(),a_word.begin(),tolower);
      }
      ++count[ a_word ];
    }
  }
  else {
    while( std::getline( file_in, a_word ) ) {  //linebased
      
      if ( to_lower ) {
	std::transform(a_word.begin(),a_word.end(),a_word.begin(),tolower);
      }
      ++count[ a_word ];
    }
  }
  file_in.close();

  // Save lexicon, and count counts, anagram values
  //
  std::ofstream file_out( output_filename.c_str(), std::ios::out );
  if ( ! file_out ) {
    l.log( "ERROR: cannot write lexicon file." );
    return -1;
  }
  std::ofstream file1_out( anaval_filename.c_str(), std::ios::out );
  if ( ! file1_out ) {
    l.log( "ERROR: cannot write anaval file." );
    return -1;
  }
  std::map<int,int> ffreqs;
  long total_count;
  for( std::map<std::string,int>::iterator iter = count.begin(); iter != count.end(); iter++ ) {
    std::string wrd = (*iter).first;
    int wfreq = (*iter).second;
    file_out << wrd << " " << wfreq << "\n";
    ffreqs[wfreq]++;
    total_count += wfreq;
    //
    // anagram value
    //
    file1_out << wrd << " " << anahash( wrd ) << "\n";
  }
  file1_out.close();
  file_out.close();

  // Save count of counts. Twice, so we can change those to c* later.
  //
  // Format of .cnt: 4 764 3.6322
  //                 | |   |
  //                 ^frequency
  //                   ^number with this frequency
  //                       ^adjusted frequency (after smooth)
  //
  std::ofstream counts_out( counts_filename.c_str(), std::ios::out );
  if ( ! counts_out ) {
    l.log( "ERROR: cannot write file." );
    return -1;
  }
  counts_out << 0 << " " << 0 << " " << 0 << "\n";
  for( std::map<int,int>::iterator iter = ffreqs.begin(); iter != ffreqs.end(); iter++ ) {
    double pMLE = (double)(*iter).first / total_count;
    int cnt = (*iter).first;
    counts_out << cnt << " " << (*iter).second << " " << cnt << "\n";
  }
  counts_out.close();

  double we = word_entropy( count );
  l.log( "w_ent = " + to_str(we) );

  c.add_kv( "lexicon", output_filename );
  l.log( "SET lexicon to "+output_filename );
  return 0;
}

// A load_lexicon, which loads it and calculates some stats. But
// where/how do we store it? Pointer to structure in c? Extra
// variable in Config? A store/load function in Config?
// Have this is a MyConfig class, inherits from Config?
//
// Maybe all lexicon related functions in lexicon.cc ?
//
int load_lexicon(Logfile& l, Config& c)  {
  l.log( "load_lexicon" );
  std::string lexicon_filename = c.get_value("lexicon");
  l.inc_prefix();
  l.log( "lexicon:  "+lexicon_filename );
  l.log( "SET: word_entropy" );
  l.dec_prefix();

}

unsigned long long anahash( std::string& s ) {
  unsigned long long res = 0;

  for ( int i = 0; i < s.length(); i++ ) {
    res += (unsigned long long)pow((unsigned long long)s[i],5);
  }
  return res;
}

// needs filename, and type.
// writes filename.hpx, if test type, we hapax target and have extra t
// in the name. This one doesn't change the filename, it adds
// trainfile or testfile depending on type!
// use set mechanism in script?? if we do "set: type:test" will that work?
//
int hapax(Logfile& l, Config& c)  {
  l.log( "hapax" );
  const std::string& filename = c.get_value( "filename" );
  int hapax = stoi( c.get_value( "hpx", "1" ));
  std::string output_filename = filename + ".hpx" + to_str(hapax);
  std::string lexicon_filename = c.get_value("lexicon");
  std::string hpx_sym = c.get_value("hpx_sym", "<unk>");
  int type = 0;
  if ( c.get_value( "type" ) == "test" ) {
    type = 1;
    output_filename += "t";
  }
  l.inc_prefix();
  l.log( "filename: "+filename );
  l.log( "hpx:      "+to_str(hapax) );
  l.log( "lexicon:  "+lexicon_filename );
  l.log( "OUTPUT:   "+output_filename );
  l.dec_prefix();

  std::ifstream file_in( filename.c_str() );
  if ( ! file_in ) {
    l.log( "ERROR: cannot load file." );
    return -1;
  }
  std::ofstream file_out( output_filename.c_str(), std::ios::out );
  if ( ! file_out ) {
    l.log( "ERROR: cannot write file." );
    return -1;
  }
  std::ifstream file_lexicon( lexicon_filename.c_str() );
  if ( ! file_lexicon ) {
    l.log( "ERROR: cannot load file." );
    return -1;
  }

  std::string a_line;
  std::vector<std::string>words;
  unsigned long lcount = 0;
  unsigned long wcount = 0;
  std::string a_word;
  int wfreq;
  std::map<std::string,int> wfreqs;

  // Read the lexicon with word frequencies.
  // Words with freq <= hapax level are skipped (i.e., will be
  // "HAPAX"/"<unk>" in the resulting file. So everything not in the
  // resulting list will be skipped.
  //
  l.log( "Reading lexicon, creating hapax list." );
  while( file_lexicon >> a_word >> wfreq ) {
    if ( wfreq > hapax ) {
      wfreqs[a_word] = wfreq;
    }
    ++wcount;
  }
  l.log( "Loaded hapax list ("+to_str((int)wfreqs.size())+"), total "
	 + to_str(wcount) );
  file_lexicon.close();

  // We don't want to hapax '_', or <s> (small chance anyway)
  //
  wfreqs["_"]    = 1;
  wfreqs["<s>"]  = 1;
  wfreqs["</s>"] = 1;

  while( std::getline( file_in, a_line )) {
    ++lcount;
    Tokenize( a_line, words, ' ' );
    //
    // The features.
    //
    for ( int i = 0; i < words.size()-1; i++ ) {
      //
      // Words not in the wfreqs vector are "HAPAX"ed.
      // (The actual freq is not checked here anymore.)
      //
      if ( wfreqs.find(words.at(i)) == wfreqs.end() ) { // not found
	file_out << hpx_sym << " ";
      } else {
	file_out << words.at(i) << " ";
      }
    }
    //
    // The target.
    //
    int idx = words.size()-1; // Target
    if ( type == 0 ) { // Train, alleen de features hapaxen.
      file_out << words.at(idx); // target
    } else {
      if ( wfreqs.find(words.at(idx)) == wfreqs.end() ) { // not found
	file_out <<  hpx_sym;
      } else {
	file_out << words.at(idx);
      }
    }
    file_out <<  std::endl;
    words.clear();
  }
  file_out.close();
  file_in.close();

  /*
  if ( type == 0 ) {
    c.add_kv( "trainfile", output_filename );
    l.log( "SET trainfile to " + output_filename );
  } else {
    c.add_kv( "testfile", output_filename );
    l.log( "SET testfile to " + output_filename );
  }
  */
  c.add_kv( "filename", output_filename );
  l.log( "SET filename to "+filename );

  return 0;
}

// Hapax one line only.
//
int hapax_line_OLD(Logfile& l, Config& c)  {
  l.log( "hapax_line" );
  int hapax = stoi( c.get_value( "hpx", "1" ));
  const std::string& lexicon_filename = c.get_value("lexicon");
  const std::string& a_line = c.get_value("classify", "error");

  // Always test.
  //
  int type = 1;
  if ( c.get_value( "type" ) == "train" ) {
    type = 1;
  }
  l.inc_prefix();
  l.log( "hpx:      "+to_str(hapax) );
  l.log( "lexicon:  "+lexicon_filename );
  l.dec_prefix();

  std::ifstream file_lexicon( lexicon_filename.c_str() );
  if ( ! file_lexicon ) {
    l.log( "ERROR: cannot load file." );
    return -1;
  }

  std::vector<std::string>words;
  unsigned long lcount = 0;
  unsigned long wcount = 0;
  std::string a_word;
  int wfreq;
  std::map<std::string,int> wfreqs;

  // Read the lexicon with word frequencies.
  // Words with freq <= hapax level are skipped (i.e., will be
  // "HAPAX" in the resulting file.
  //
  l.log( "Reading lexicon, creating hapax list." );
  while( file_lexicon >> a_word >> wfreq ) {
    if ( wfreq > hapax ) {
      wfreqs[a_word] = wfreq;
    }
  }
  l.log( "Loaded hapax list ("+to_str((int)wfreqs.size())+")" );
  file_lexicon.close();

  std::string res;
  Tokenize( a_line, words, ' ' );

  for ( int i = 0; i < words.size()-1; i++ ) {
    //
    // NB: unknown words and "_" are hapaxed as well.
    // Words not in the wfreqs vector are "HAPAX"ed.
    //
    if ( wfreqs.find(words.at(i)) == wfreqs.end() ) { // not found
      res = res + "HAPAX ";
    } else {
      res = res + words.at(i) + " ";
    }
  }
  //
  // The target.
  //
  int idx = words.size()-1; // Target
  if ( type == 0 ) { // Train, alleen de features hapaxen.
    res = res + words.at(idx); // target
  } else {
    if ( wfreqs.find(words.at(idx)) == wfreqs.end() ) { // not found
      res = res +  "HAPAX";
    } else {
      res = res + words.at(idx);
    }
  }

  // And set the classify line to the hapaxed result.
  //
  l.log( res );
  c.add_kv( "classify", res );

  return 0;
}

/*
int trainfile(Logfile& l, Config& c)  {
  const std::string& filename = c.get_value( "filename" );
  c.add_kv( "trainfile", filename );
  l.log( "trainfile: "+filename );
  return 0;
}

int testfile(Logfile& l, Config& c)  {
  const std::string& filename = c.get_value( "filename" );
  c.add_kv( "testfile", filename );
  l.log( "testfile: "+filename );
  return 0;
}
*/

int trainfile(Logfile& l, Config& c)  {
  l.log( "trainfile" );
  const std::string& filename = c.get_value( "trainfile" );
  c.add_kv( "filename", filename );
  l.log( "SET filename to "+filename );
  return 0;
}

int testfile(Logfile& l, Config& c)  {
  l.log( "testfile" );
  const std::string& filename = c.get_value( "testfile" );
  c.add_kv( "filename", filename );
  l.log( "SET filename to "+filename );
  return 0;
}

// a timbl_test_with_ibase

// make data from usenet: target is per np.nnn hetzelfde,... 

// parameter: filename
//            ws
//
int window_usenet( Logfile& l, Config& c ) {
  l.log( "window_usenet" );
  const std::string& filename        = c.get_value( "filename" );
  int                ws              = stoi( c.get_value( "ws", "3" ));
  std::string        output_filename = filename + ".ws" + to_str(ws);
  l.inc_prefix();
  l.log( "filename: "+filename );
  l.log( "ws:       "+to_str(ws) );
  l.log( "OUTPUT:   "+output_filename );
  l.dec_prefix();

  std::ifstream file_in( filename.c_str() );
  if ( ! file_in ) {
    l.log( "ERROR: cannot load file." );
    return -1;
  }
  std::ofstream file_out( output_filename.c_str(), std::ios::out );
  if ( ! file_out ) {
    l.log( "ERROR: cannot write file." );
    return -1;
  }

  std::string               a_word;
  std::string               target;
  std::vector<std::string>  window(ws+1, "_");
  std::map<std::string,int> count;
  std::vector<std::string>::iterator vi;
  std::ostream_iterator<std::string> output( file_out, " " );
  std::vector<std::string>  data;

  // Look for target first?
  // TARGET:value
  // lees tot volgende target, and window_line die array?
  while( file_in >> a_word ) {

    if ( a_word.substr(0,7) == "TARGET:" ) {
      //window data with target
      if ( data.size() > 0 ) {
	std::vector<std::string>  window(ws, "_");
	for ( int i = 0; i < data.size(); i++ ) {
	  std::copy( window.begin(), window.end(), output );
	  file_out << target << std::endl;
	  copy( window.begin()+1, window.end(), window.begin() );
	  window.at(ws-1) = data.at(i);
	}
	std::copy( window.begin(), window.end(), output );
	file_out << target << std::endl;
      }
      target = a_word.substr(7);
      data.clear();
      continue;
    }
    data.push_back( a_word );
    /*
    std::transform(a_word.begin(),a_word.end(),a_word.begin(),tolower);
    std::copy( window.begin(), window.end()-1, output );
    file_out << target << std::endl;
    window.at(ws) = a_word;
    copy( window.begin()+1, window.end(), window.begin() );
    */
  }
  file_out.close();
  file_in.close();

  c.add_kv( "filename", output_filename );
  l.log( "SET filename to "+output_filename );
  return 0;
}


// ./wopr --run server -p ibasefile:tekst.txt.l1000.ws7.ibase,timbl:"-a1 +D"
//
#ifdef TIMBL
int server(Logfile& l, Config& c) {
  const std::string& timbl  = c.get_value("timbl");

  try {
    Timbl::TimblAPI *My_Experiment = new Timbl::TimblAPI( timbl );
    (void)My_Experiment->GetInstanceBase( c.get_value( "ibasefile" ));
    My_Experiment->StartServer( 8888, 1 );
  }
  catch ( const std::exception& e ) {
    l.log( "ERROR: exception caught." );
    return -1;
  }

  return 0;
}
#else
int server( Logfile& l, Config& c ) {
  l.log( "No TIMBL support." );
  return -1;
}
#endif

// test spul
// ./wopr -r test -p zin:"1 2 3"
//
int test(Logfile& l, Config& c) {
  std::string foo = c.get_value( "zin" );
  std::vector<std::string> results;
  std::vector<std::string>::iterator ri;

  l.log( "ngram( 3 )" );
  ngram_line( foo, 3, results );
  for ( ri = results.begin(); ri != results.end(); ri++ ) {
    l.log( *ri );
  }
  results.clear();
  l.log( "ngram( 1 )" );
  ngram_line( foo, 1, results );
  for ( ri = results.begin(); ri != results.end(); ri++ ) {
    l.log( *ri );
  }
  results.clear();
  l.log( "ngram( 5 )" );
  ngram_line( foo, 5, results );
  for ( ri = results.begin(); ri != results.end(); ri++ ) {
    l.log( *ri );
  }
  results.clear();

  l.log( "window( 7, 0, false )" );
  window( foo, "T T T T T T T", 7, 0, false, results );
  for ( ri = results.begin(); ri != results.end(); ri++ ) {
    l.log( *ri );
  }
  results.clear();
  l.log( "window( 7, 0, true )" );
  window( foo, "T T T T T T T", 7, 0, true, results );
  for ( ri = results.begin(); ri != results.end(); ri++ ) {
    l.log( *ri );
  }
  results.clear();

  l.log( "window( 3, 0 ) + target, backoff 0" );
  window( foo, foo, 3, 0, 0, results );
  for ( ri = results.begin(); ri != results.end(); ri++ ) {
    l.log( *ri );
  }
  results.clear();
  l.log( "window( 3, 0 ) + target, backoff 1" );
  window( foo, foo, 3, 0, 1, results );
  for ( ri = results.begin(); ri != results.end(); ri++ ) {
    l.log( *ri );
  }
  results.clear();
  l.log( "window( 2, 0 ) + target, backoff 1" );
  window( foo, foo, 2, 0, 1, results );
  for ( ri = results.begin(); ri != results.end(); ri++ ) {
    l.log( *ri );
  }
  results.clear();

  l.log( "window( 2, 3, false )" );
  window( foo, foo, 2, 3, false, results );
  for ( ri = results.begin(); ri != results.end(); ri++ ) {
    l.log( *ri );
  }
  results.clear();
  l.log( "window( 2, 3, true )" );
  window( foo, foo, 2, 3, true, results );
  for ( ri = results.begin(); ri != results.end(); ri++ ) {
    l.log( *ri );
  }
  results.clear();
  l.log( "window( 1, 1, false )" );
  window( foo, "", 1, 1, false, results );
  for ( ri = results.begin(); ri != results.end(); ri++ ) {
    l.log( *ri );
  }
  results.clear();
  l.log( "window( 0, 3, false )" );
  window( foo, "", 0, 3, false, results );
  for ( ri = results.begin(); ri != results.end(); ri++ ) {
    l.log( *ri );
  }


  // Andere test
  //
  std::cout << "log2" << std::endl;
  double slp = 
    log2( 0.000769644 ) + 
    log2( 0.0513096 ) +
    log2( 0.256548 ) + 
    log2( 0.0855161 ) +
    log2( 0.0934579 ) +
    log2( 0.00159902 ) +
    log2( 0.769644 ) +
    log2( 0.361747 ) +
    log2( 0.722171 );
  std::cout << slp << std::endl;
  slp = slp / 9;
  std::cout << slp << std::endl;
  double ppl = pow( 2, -slp );
  std::cout << ppl << std::endl;

  std::cout << "log10" << std::endl;
  slp =
    log10( 0.000769644 ) + 
    log10( 0.0513096 ) +
    log10( 0.256548 ) + 
    log10( 0.0855161 ) +
    log10( 0.0934579 ) +
    log10( 0.00159902 ) +
    log10( 0.769644 ) +
    log10( 0.361747 ) +
    log10( 0.722171 );
  std::cout << slp << std::endl;
  slp = slp / 9;
  std::cout << slp << std::endl;
  ppl = pow( 10, -slp );
  std::cout << ppl << std::endl;

  return 0;
}

/*
double log2( double n ) {
  return log(n) / log(2); // make that a constant.
}
*/

// word entropy.
// over map/array with words+freq.
//   std::map<std::string,int> wfreqs;
//   wfreqs[a_word] = wfreq;
double word_entropy( std::map<std::string,int>& wfreqs ) {
  double word_e = 0.0;
  unsigned long t_freq = 0;
  std::map<std::string,int>::iterator wi;

  for ( wi = wfreqs.begin(); wi != wfreqs.end(); wi++ ) {
    t_freq += (*wi).second;
  }
  for ( wi = wfreqs.begin(); wi != wfreqs.end(); wi++ ) {
    double w_prob = (double)((*wi).second / (double)t_freq);
    word_e += (w_prob * log2( w_prob ));
  }
  return -word_e;

}

// Maybe a version which takes the cnt array instead of the lexicon.
// THIS IS THE VERSION USED!
//
int smooth_old( Logfile& l, Config& c ) {
  l.log( "smooth" );
  const std::string& counts_filename = c.get_value( "counts" );
  const int precision = stoi( c.get_value( "precision", "6" ));
  int k = stoi( c.get_value( "k", "10" ));
  std::string gt_filename = counts_filename + ".gt";
  l.inc_prefix();
  l.log( "counts: " + counts_filename );
  l.log( "k:      " + to_str(k) );
  l.log( "OUTPUT: " + gt_filename );
  l.dec_prefix();

  std::ifstream file_counts( counts_filename.c_str() );
  if ( ! file_counts ) {
    l.log( "ERROR: cannot load counts file." );
    return -1;
  }

  std::string a_line;
  std::vector<std::string>words;
  unsigned long lcount = 0;
   
  std::string a_word;
  int wfreq;
  std::map<std::string,int> wfreqs;
  std::map<int,int> ffreqs; // sort order?

  // Read the counts of counts.
  //
  l.log( "Reading counts." );
  int count;
  int Nc0;
  double Nc1; // this is c*
  int numcounts = 0;
  int maxcounts = 0;
  unsigned long total_count = 0;
  while( file_counts >> count >> Nc0 >> Nc1 ) {
    ffreqs[count] = Nc0;
    total_count += count*Nc0;
    ++numcounts;
    if ( count > maxcounts ) {
      maxcounts = count;
    }
  }
  file_counts.close();
  l.log( "Read counts." );

  if ( k == 0 ) {
    k = numcounts;
  }

  std::map<int,int>::iterator fi;
  std::vector<int> counts;
  for ( fi = ffreqs.begin(); fi != ffreqs.end(); fi++ ) {
    /*
    if ( (int)(*fi).first < k ) {
      l.log( to_str((int)(*fi).first) + "," + to_str((int)(*fi).second) );
    }
    */
    //counts.at( (int)(*fi).first ) = (int)(*fi).second;;
  }
  l.log( "Total count: " + to_str( total_count ));

  std::ofstream counts_out( counts_filename.c_str(), std::ios::out );
  if ( ! counts_out ) {
    l.log( "ERROR: cannot write file." );
    return -1;
  }

  // GT smoothed file, contains:
  // freq, number of words with freq, pMLE, pMLE smoothed.
  //
  std::ofstream file_out( gt_filename.c_str(), std::ios::out );
  if ( ! file_out ) {
    l.log( "ERROR: cannot write file." );
    return -1;
  }

  // N1/N
  double p0 = (double)ffreqs[1] / (double)total_count;
  l.log( "N1/N = " + to_str(ffreqs[1]) + "/" + to_str(total_count) + " = " + to_str( p0 ) + " " + to_str( p0 / (double)total_count ));
  counts_out << "0 0 " << p0 << std::endl; // This is a probability, not c*

  // GoodTuring-3.pdf
  //
  double GTfactor = 1 - p0; // p0 =  P(new_word)
  file_out << "0 0 0 " << p0 << std::endl;

  // Calculate new counts Good-Turing
  // ffreqs contains the counts.
  //
  // NB: See GoodTuring-3.pdf !
  //
  for ( int i = 1; i < k; i++ ) { // This loop should be with iterator
    int Nc = (int)ffreqs[i]; // PJB: Previously smoothed value??? 
    int cp1 = i+1;                // Should be fi.next
    int Ncp1 = (int)ffreqs[cp1];
    //
    // pMLE = probability of word occuring Nc times
    //
    double pMLE = Nc / (double)total_count; //PJB Nc or i ?? (refAA)
    double c_star = i; // this is really wrong...
    if ( Nc != 0 ) {
      c_star = (double)cp1 * (double)( (double)Ncp1 / (double)Nc );
    }
    double p_star = pMLE; // average between prev and next?
    if ( c_star != 0 ) {
      //p_star = (double)c_star / (double)total_count; //(see refAA)
      p_star = pMLE * GTfactor; // The adjusted pMLE for words occuring Nc times SHould be re-calculated every time?
    }
    //l.log( "c="+to_str(i)+" Nc="+to_str(Nc)+" Nc+1="+to_str(Ncp1)+" c*="+to_str(c_star, precision)+" pMLE="+to_str(pMLE, precision)+" p*="+to_str(p_star, precision) );
l.log( "c="+to_str(i)+" Nc="+to_str(Nc)+" Nc+1="+to_str(Ncp1)+" c*="+to_str(c_star, precision) );
    counts_out << i << " " << Nc << " " << c_star << std::endl;
    file_out << i << " " << Nc << " " << pMLE << " " << p_star;
    file_out << " " << c_star/(double)total_count << std::endl;
    //note 0s
    if ( c_star == 0 ) {
      l.log( "WARNING: too little data.");
    }
  }

  // Stämmer inte - we need the *first and *second!
  //
  for ( int i = k; i <= maxcounts; i++ ) { // niet numcounts, te weinig!
    int Nc = (int)ffreqs[i];
    double pMLE = Nc / (double)total_count;
    counts_out << i << " " << Nc << " " << i << std::endl;
    if ( Nc > 0 ) {
      file_out << i << " " << Nc << " " << pMLE << " " << pMLE;
      file_out << " " << i/(double)total_count << std::endl; //note 0s
    }
  }
  file_out.close();
  counts_out.close();

  return 0;  
}

// Maybe a version which takes the cnt array instead of the lexicon.
//
int smooth_old_LEX( Logfile& l, Config& c ) {
  l.log( "smooth" );
  const std::string& lexicon_filename = c.get_value( "lexicon" );
  const int precision = stoi( c.get_value( "precision", "6" ));
  const int k = stoi( c.get_value( "k", "10" ));
  l.inc_prefix();
  l.log( "lexicon: " + lexicon_filename );
  l.log( "k:       " + to_str(k) );
  l.dec_prefix();

  std::ifstream file_lexicon( lexicon_filename.c_str() );
  if ( ! file_lexicon ) {
    l.log( "ERROR: cannot load file." );
    return -1;
  }

  std::string a_line;
  std::vector<std::string>words;
  unsigned long lcount = 0;
  unsigned long total_count = 0;
  std::string a_word;
  int wfreq;
  std::map<std::string,int> wfreqs;
  std::map<int,int> ffreqs;

  for ( int i = 0; i < 10; i++ ) {
    ffreqs[i] = 0;
  }

  // Read the lexicon with word frequencies.
  // We need a hash with frequence - countOfFrequency, ffreqs.
  //
  l.log( "Reading lexicon." );
  while( file_lexicon >> a_word >> wfreq ) {
    wfreqs[a_word] = wfreq;
    ffreqs[wfreq]++;
    total_count += wfreq;
  }
  file_lexicon.close();
  l.log( "Read lexicon." );

  std::map<int,int>::iterator fi;
  std::vector<int> counts;
  for ( fi = ffreqs.begin(); fi != ffreqs.end(); fi++ ) {
    l.log( to_str((int)(*fi).first) + "," + to_str((int)(*fi).second) );
    //counts.at( (int)(*fi).first ) = (int)(*fi).second;;
  }
  l.log( "Total count: " + to_str( total_count ));

  // N1/N
  double p0 = (double)ffreqs[1] / (double)total_count ;
  l.log( "N1/N = " + to_str( p0 ));

  // Calculate new counts Good-Turing
  // ffreqs contains the counts.
  //
  //std::vector<int> c_star;
  for ( int i = 1; i < k; i++ ) {
    int Nc = (int)ffreqs[i];
    int cp1 = i+1;
    int Ncp1 = (int)ffreqs[cp1];
    double pMLE = i / (double)total_count; //PJB Nc or i ??
    double c_star = i;
    if ( Nc != 0 ) {
      c_star = (double)cp1 * (double)( (double)Ncp1 / (double)Nc );
    }
    double p_star = pMLE; // average between prev and next?
    if ( c_star != 0 ) {
      p_star = (double)c_star / (double)total_count;
    }
    l.log( "c="+to_str(i)+" Nc="+to_str(Nc)+" Nc+1="+to_str(Ncp1)+" c*="+to_str(c_star, precision)+" pMLE="+to_str(pMLE, precision)+" p*="+to_str(p_star, precision) );
    //std::cerr << i << "," << i << "," << c_star << std::endl;
    std::cerr << i << "," << to_str(pMLE,precision) << "," << to_str(p_star,precision) << std::endl;
  }

  return 0;  
}

int smooth(Logfile& l, Config& c)  {
  l.log( "smooth" );
  const std::string& filename = c.get_value( "filename" );
  l.inc_prefix();
  l.log( "filename: "+filename );
  l.dec_prefix();

  std::ifstream file_in( filename.c_str() );
  if ( ! file_in ) {
    l.log( "ERROR: cannot load file." );
    return -1;
  }

  std::string a_line;
  std::vector<std::string> words;
  unsigned long lcount = 0;
  unsigned long total_count = 0;
  unsigned long word_count = 0;
  std::string a_word;
  int wfreq;
  std::map<std::string,int> pcounts; // pattern counter
  std::map<int,int> ffreqs; // frequency of frequencies

  std::map<std::string,int> class_count;

  std::vector< std::map<std::string,int> > foobar;

  l.log( "Reading data." );
  while( std::getline( file_in, a_line )) {

    Tokenize( a_line, words, ' ' );
    a_line = "";
    int last = words.size()-1;
    for ( int i = 0; i < last; i++ ) {
      //(foobar.at(i))[words.at(i)] += 1;
      a_line = a_line + words.at(i) + ' ';
    }
    class_count[words.at(last)]++;
    words.clear();
    //l.log( a_line );
    pcounts[a_line]++;
    lcount++;
  }
  file_in.close();
  l.log( "Line count: " + to_str(lcount));

  std::map<std::string,int>::iterator pi;
  for ( pi = pcounts.begin(); pi != pcounts.end(); pi++ ) {
    int freq = (int)(*pi).second;
    ffreqs[freq]++;
  }
  for ( pi = class_count.begin(); pi != class_count.end(); pi++ ) {
    //l.log( (*pi).first + "," + to_str((int)(*pi).second) );
  }
  std::map<int,int>::iterator fi;
  for ( fi = ffreqs.begin(); fi != ffreqs.end(); fi++ ) {
    l.log( to_str((int)(*fi).first) + "," + to_str((int)(*fi).second) );
  }

  l.log( "Number of counts: " + to_str( static_cast<long>(ffreqs.size()) ));
  l.log( "Number of classes: " + to_str( static_cast<long>(class_count.size())));
  l.log( "Word count (lexicon size): " + to_str( word_count ));
  l.log( "Total count: " + to_str( total_count ));
  
  // Size of possible data space
  //
  unsigned long possible_patterns = pow( word_count, 7 );
  l.log( "Possible patterns: " + to_str( possible_patterns ));

  // N1/N
  double n1_over_n = (double)ffreqs[1] / (double)total_count ;
  l.log( "N1/N = " + to_str( n1_over_n ));

  unsigned long data_size = 0; // same as total_count
  while( std::getline( file_in, a_line )) {
    ++data_size;
    Tokenize( a_line, words, ' ' );
    //
    // The features.
    //
    for ( int i = 0; i < words.size()-1; i++ ) {
      //
    }
    //
    // The target.
    //
    int idx = words.size()-1; // Target
  }
  file_in.close();

  l.log( "Data size: " + to_str( data_size ));

  // unseen_est = (n1/n) / (possible_patterns - data_size)

  double foo = n1_over_n / ( possible_patterns - data_size );
  l.log( "foo: " + to_str( foo ));

  return 0;
}

int read_a3(Logfile& l, Config& c) {
  l.log( "read_a3" );
  const std::string& filename = c.get_value( "filename" );
  std::string output_filename = filename + ".xa3"; // ex a3 format.
  l.inc_prefix();
  l.log( "filename: "+filename );
  l.log( "OUTPUT:   "+output_filename );
  l.dec_prefix();

  std::ifstream file_in( filename.c_str() );
  if ( ! file_in ) {
    l.log( "ERROR: cannot load file." );
    return -1;
  }

  std::ofstream file_out( output_filename.c_str(), std::ios::out );
  if ( ! file_out ) {
    l.log( "ERROR: cannot write file." );
    file_in.close();
    return -1;
  }
  
  // Lines like:
  // NULL ({ }) Approval ({ 1 }) of ({ 2 }) the ({ 3 }) Minutes ({ 4 }) 
  //          ^posb      |
  //                     ^pose
  std::string a_line;
  std::string ssym = "<s>";
  std::string esym = "</s>";
  while( getline( file_in, a_line) ) {
    if ( a_line.substr(0,4) == "NULL" ) {
      //l.log( a_line );
      std::string clean;
      std::string::iterator si;
      
      size_t posb = 0;
      size_t pose = 0;
      std::string delb = "})";
      std::string dele = "({";

      posb = a_line.find( delb, pose );
      pose = a_line.find( dele, posb );
      file_out << ssym << " ";
      while ( (posb != std::string::npos) && (pose != std::string::npos) ) {
	std::string word = a_line.substr(posb+3, pose-posb-4);
	file_out << word << " "; // " " was std::endl;
	posb = a_line.find(delb, pose);
	pose = a_line.find(dele, posb);
      }
      file_out << esym << std::endl;
    }
  }
  file_out.close();
  file_in.close();

  c.add_kv( "filename", output_filename );
  l.log( "SET filename to "+output_filename );  
  return 0;
}

/*
  Calculate perplexiteit of a sentence (per sentence, file).

  Bleh about <s> </s> ?

  this was window( l, c ), maybe modify window_s( l, c ) ? done

  If we want to choose different ibases: we need some extra info (window
  size, ...). How? Meta info in the ibase file? Extra file with new 
  extension? Specify an extra config file?

  Simpeler: calculate pplx on windowed Timbl output file?

  TEST: ../wopr -r pplx -p filename:zin1.txt,lexicon:reuters.martin.tok.1000.lex,ibasefile:reuters.martin.tok.1000.ws3.ibase,ws:3,timbl:"-a1 +D"

  TODO: pplx_simple: no windowing, take data file as 'pre made' but
        use Timbl to classif and calculate pplx.
*/
#ifdef TIMBL
int pplx( Logfile& l, Config& c ) {
  l.log( "pplx" );
  const std::string& filename         = c.get_value( "filename" );
  const std::string& ibasefile        = c.get_value( "ibasefile" );
  const std::string& lexicon_filename = c.get_value( "lexicon" );
  const std::string& timbl            = c.get_value( "timbl" );
  int                ws               = stoi( c.get_value( "ws", "3" ));
  bool               to_lower         = stoi( c.get_value( "lc", "0" )) == 1;
  std::string        output_filename  = filename + ".px" + to_str(ws);
  std::string        pre_s            = c.get_value( "pre_s", "<s>" );
  std::string        suf_s            = c.get_value( "suf_s", "</s>" );
  int                skip             = 0;
  Timbl::TimblAPI   *My_Experiment;
  std::string        distrib;
  std::vector<std::string> distribution;
  std::string        result;
  double             distance;

  l.inc_prefix();
  l.log( "filename:   "+filename );
  l.log( "ibasefile:  "+ibasefile );
  l.log( "lexicon:    "+lexicon_filename );
  l.log( "timbl:      "+timbl );
  l.log( "ws:         "+to_str(ws) );
  l.log( "lowercase:  "+to_str(to_lower) );
  l.log( "OUTPUT:     "+output_filename );
  l.dec_prefix();

  std::ifstream file_in( filename.c_str() );
  if ( ! file_in ) {
    l.log( "ERROR: cannot load inputfile." );
    return -1;
  }
  std::ofstream file_out( output_filename.c_str(), std::ios::out );
  if ( ! file_out ) {
    l.log( "ERROR: cannot write output file." );
    return -1;
  }

  // Load lexicon. NB: hapaxed lexicon is different? Or add HAPAX entry?
  //
  std::ifstream file_lexicon( lexicon_filename.c_str() );
  if ( ! file_lexicon ) {
    l.log( "ERROR: cannot load lexicon file." );
    return -1;
  }
  // Read the lexicon with word frequencies.
  // We need a hash with frequence - countOfFrequency, ffreqs.
  //
  l.log( "Reading lexicon." );
  std::string a_word;
  int wfreq;
  unsigned long total_count = 0;
  std::map<std::string,int> wfreqs; // whole lexicon
  while ( file_lexicon >> a_word >> wfreq ) {
    wfreqs[a_word] = wfreq;
    total_count += wfreq;
  }
  file_lexicon.close();
  l.log( "Read lexicon (total_count="+to_str(total_count)+")." );

  try {
    My_Experiment = new Timbl::TimblAPI( timbl );
    (void)My_Experiment->GetInstanceBase( ibasefile );
    // My_Experiment->Classify( cl, result, distrib, distance );
  } catch (...) {}

  std::vector<std::string>::iterator vi;
  std::ostream_iterator<std::string> output( file_out, " " );

  std::string a_line;
  std::vector<std::string> results;
  std::vector<std::string> targets;
  std::vector<std::string>::iterator ri;
  const Timbl::ValueDistribution *vd;
  const Timbl::TargetValue *tv;

  skip = 0; //kludge factor

  while( std::getline( file_in, a_line )) {

    if ( to_lower ) {
      std::transform(a_line.begin(),a_line.end(),a_line.begin(),tolower); 
    }

    a_line = pre_s + ' ' + a_line + ' ' + suf_s;
    
    std::string wopr_line;

    window( a_line, a_line, ws, 0, false, results ); 

    if ( (skip == 0) || (results.size() >= ws) ) {
      for ( ri = results.begin()+skip; ri != results.end(); ri++ ) {
	std::string cl = *ri;
	file_out << cl << std::endl;
	l.log( cl );
	//
	// This pattern should be tested now (or write file first and
	// then test). So we need an ibase, and maybe write extra info when
	// we test (so we can calculate pplx). Better to test directly like
	// in server2.
	// Beginning of sentence, smaller contexts? How does srilm do it?
	// Prepare the instances FIRST, stuff into an array (so we can have
	// small ones at the beginning), then loop over and classify.
	// The special cases are at the beginning of the sentence.
	// 0) we start with nothing - predict first word is meaningless.
	//    (classifier with right context? :-) )
	// 1) shorter context till we end up at ws
	// 2) if right-most word in context is <unk>, yet a different classifier
	//    again.
	// class with word+what to do.
	// eg: ( 'the', '', first-word-classifier )
	//     ( 'brown', 'the quick', ws-2-ibase-classifier )
	//
	// we can do this for MT: different classifiers per word, train 
	// classifier to get right classifier for each word. Uhm
	//
	// --
	// Extra <k> <unk> pattern parallel to word pattern?
	// Or let the <known><unk> pattern choose the ibase:
	//  <k><k><u><T> selects a ws2.ibase predicting word+2
	// --
	//
	tv = My_Experiment->Classify( cl, vd );
	wopr_line = wopr_line + tv->Name() + " ";
	l.log( "Answer: " + tv->Name() );

	Timbl::ValueDistribution::dist_iterator it = vd->begin();
	int cnt = 0;
	cnt = vd->size();
	//l.log( to_str(cnt) );
	if ( cnt < 25 ) {
	  it = vd->begin();
	  while ( it != vd->end() ) {
	    //std::cout << cnt << " " << it->second->Value() << ":  "
	    //	       << it->second->Weight() << std::endl;
	    std::cout << it->second << ' ';
	    ++it;
	  }
	  std::cout << std::endl;
	}
	
      }
      results.clear();
    } else {
      l.log( "SKIP: " + a_line );
    }

    l.log( wopr_line );
  }

  file_out.close();
  file_in.close();

  c.add_kv( "filename", output_filename );
  l.log( "SET filename to "+output_filename );
  return 0;
}
#else
int pplx( Logfile& l, Config& c ) {
  l.log( "No TIMBL support." );
  return -1;
}  
#endif

// wopr -r pplxs -p filename:reuters.martin.tok.1000.ws3,lexicon:reuters.martin.tok.1000.lex,ibasefile:reuters.martin.tok.1000.ws3.ibase,ws:3,timbl:"-a1 +D"
// wopr -r pplxs -p filename:test1.ws3.hpx5,ibasefile:ep_sv_cl.ws3.hpx5_de.ibase,timbl:'-a1 +D',lexicon:ep_sv_cl.lex,hpx:5
//
#ifdef TIMBL
struct distr_elem {
  std::string name;
  double      freq;
  double      s_freq;
  bool operator<(const distr_elem& rhs) const {
    return freq > rhs.freq;
  }
};
struct cached_distr {
  int cnt;
  long sum_freqs;
  double entropy;
  std::map<std::string,int> freqs; // word->frequency
  std::vector<distr_elem> distr_vec; // top-n to print
  bool operator<(const cached_distr& rhs) const {
    return cnt > rhs.cnt;
  }
};
int pplx_simple( Logfile& l, Config& c ) {
  l.log( "pplxs" );
  const std::string& filename         = c.get_value( "filename" );
  const std::string& ibasefile        = c.get_value( "ibasefile" );
  const std::string& lexicon_filename = c.get_value( "lexicon" );
  const std::string& counts_filename  = c.get_value( "counts" );
  const std::string& timbl            = c.get_value( "timbl" );
  int                hapax            = stoi( c.get_value( "hpx", "0" ));
  std::string        id               = c.get_value( "id", to_str(getpid()) );
  int                ws               = stoi( c.get_value( "ws", "3" ));
  int                lc               = stoi( c.get_value( "lc", "0" ));
  int                rc               = stoi( c.get_value( "rc", "0" ));
  std::string        output_filename  = filename + "_" + id + ".px";
  std::string        output_filename1 = filename + "_" + id + ".pxs";
  std::string        pre_s            = c.get_value( "pre_s", "<s>" );
  std::string        suf_s            = c.get_value( "suf_s", "</s>" );
  int                topn             = stoi( c.get_value( "topn", "0" ) );
  int                cache_size       = stoi( c.get_value( "cache", "3" ) );
  int                cache_threshold  = stoi( c.get_value( "cth", "25000" ) );
  int                skip             = 0;
  Timbl::TimblAPI   *My_Experiment;
  std::string        distrib;
  std::vector<std::string> distribution;
  std::string        result;
  double             distance;

  // This is better for l0r3 contexts &c.
  // It should really only say the length of the context, i.e the
  // number of features before the target.
  //
  ws = lc + rc;

  // Sanity check.
  //
  if ( cache_size <= 0 ) {
    l.log( "WARNING: cache_size should be > 0, setting to 1." );
    cache_size = 1;
  }
  l.inc_prefix();
  l.log( "filename:       "+filename );
  l.log( "ibasefile:      "+ibasefile );
  l.log( "lexicon:        "+lexicon_filename );
  l.log( "counts:         "+counts_filename );
  l.log( "timbl:          "+timbl );
  l.log( "hapax:          "+to_str(hapax) );
  l.log( "ws:             "+to_str(ws) );
  l.log( "lc:             "+to_str(lc) );
  l.log( "rc:             "+to_str(rc) );
  l.log( "topn:           "+to_str(topn) );
  l.log( "cache:          "+to_str(cache_size) );
  l.log( "cache threshold:"+to_str(cache_threshold) );
  l.log( "id:             "+id );
  l.log( "OUTPUT:         "+output_filename );
  l.log( "OUTPUT:         "+output_filename1 );
  l.dec_prefix();

  if ( file_exists(l,c,output_filename) && file_exists(l,c,output_filename1) ) {
    l.log( "OUTPUT files exist, not overwriting." );
    return 0;
  }

  std::ifstream file_in( filename.c_str() );
  if ( ! file_in ) {
    l.log( "ERROR: cannot load inputfile." );
    return -1;
  }
  std::ofstream file_out( output_filename.c_str(), std::ios::out );
  if ( ! file_out ) {
    l.log( "ERROR: cannot write .px output file." ); // for px
    return -1;
  }
  file_out << "# instance+target classification logprob entropy word_lp guess (dist.cnt [topn])" << std::endl;

  std::ofstream file_out1( output_filename1.c_str(), std::ios::out );
  if ( ! file_out1 ) {
    l.log( "ERROR: cannot write .pxs output file." ); // for pxs
    return -1;
  }
  file_out1 << "# nr. #words sum(logprob) avg.pplx avg.wordlp std.dev(wordlp) [wordlp(each word)]" << std::endl;

  // Load lexicon. NB: hapaxed lexicon is different? Or add HAPAX entry?
  //
  int wfreq;
  unsigned long total_count = 0;
  unsigned long N_1 = 0; // Count for p0 estimate.
  std::map<std::string,int> wfreqs; // whole lexicon
  std::ifstream file_lexicon( lexicon_filename.c_str() );
  if ( ! file_lexicon ) {
    l.log( "NOTICE: cannot load lexicon file." );
    //return -1;
  } else {
    // Read the lexicon with word frequencies.
    // We need a hash with frequence - countOfFrequency, ffreqs.
    //
    l.log( "Reading lexicon." );
    std::string a_word;
    while ( file_lexicon >> a_word >> wfreq ) {
      if ( wfreq > hapax ) { // PJB: klopt dit? We don't get ++N_1? Smoothing?
	wfreqs[a_word] = wfreq;
	total_count += wfreq;
	if ( wfreq == 1 ) { // Maybe <= hapax ?, outside the if >hapax loop
	  ++N_1;
	}
      }
    }
    file_lexicon.close();
    l.log( "Read lexicon (total_count="+to_str(total_count)+")." );
  }
  
  // Beginning of sentence marker.
  // Maybe should just be a parameter...
  //
  std::string bos = "";
  for ( int i = 0; i < ws; i++ ) {
    bos = bos + "_ ";
  }

  // If we want smoothed counts, we need this file...
  // Make mapping <int, double> from c to c* ?
  //
  std::map<int,double> c_stars;
  int Nc0;
  double Nc1; // this is c*
  int count;

  std::ifstream file_counts( counts_filename.c_str() );
  if ( ! file_counts ) {
    l.log( "NOTICE: cannot read counts file, no smoothing will be applied." ); 
  } else {
    l.log( "Reading counts." );
    while( file_counts >> count >> Nc0 >> Nc1 ) {
      c_stars[count] = Nc1;
    }
    file_counts.close();
    l.log( "Read counts." );
  }

  // The P(new_word) according to GoodTuring-3.pdf
  // We need the filename.cnt for this, because we really need to
  // discount the rest if we assign probability to the unseen words.
  //
  // We need to esitmate the total number of unseen words. Same as
  // vocab, i.e assume we saw half? Ratio of N_1 to total_count?
  //
  // We need to load .cnt file as well...
  //
  double p0 = 0.00001; // Arbitrary low probability for unknown words.
  if ( total_count > 0 ) { // Better estimate if we have a lexicon
    p0 = (double)N_1 / (double)total_count;
    // Assume N_0 equals N_1...
    p0 = p0 / (double)N_1;
  }
  l.log( "P(0) = " + to_str(p0) );

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

  std::string a_line;
  std::vector<std::string> results;
  std::vector<std::string> targets;
  std::vector<std::string>::iterator ri;
  const Timbl::ValueDistribution *vd;
  const Timbl::TargetValue *tv;
  std::vector<std::string> words;
  std::vector<double> w_pplx;
  int correct = 0;
  int wrong   = 0;
  int correct_unknown = 0;
  int correct_distr = 0;
    
  // Recognise <s> or similar, reset pplx calculations.
  // Output results on </s> or similar.
  // Or a divisor which is not processed?
  //
  double sentence_prob      = 0.0;
  double sum_logprob        = 0.0;
  double sum_wlp            = 0.0; // word level pplx
  int    sentence_wordcount = 0;
  int    sentence_count     = 0;
  
  // Cache a map(string:freq) of the top-n distributions returned
  // by Timbl.
  // We get the size first, if it is bigger than the ones we have cached,
  // we can save the map when we cycle through the distribution. Next
  // time we can use the map to check if the target is in there.
  // What do we need?
  //   map with word->freq
  //   sum(freqs)
  //   number of entries
  //
  // Distribution cache
  //
  int lowest_cache = 0; // size of distr. (prolly need a higher starting value)
  std::vector<cached_distr> distr_cache;
  for ( int i = 0; i < cache_size; i++ ) {
	cached_distr c;
	c.cnt       = 0;
	c.sum_freqs = 0;
	c.entropy   = 0.0;
	distr_cache.push_back( c );
  }

  long timbl_time = 0;

  while( std::getline( file_in, a_line )) {

    words.clear();
    a_line = trim( a_line );
    Tokenize( a_line, words, ' ' );

    if ( words.size() == 1 ) { // For Hermans data. TODO: better fix.
      words.clear();
      Tokenize( a_line, words, '\t' );
    }
    std::string target = words.at( words.size()-1 );

    // Check if "bos" here. If so, we need to calculate some
    // averages, and reset the sum/counting variables.
    //
    // We also need this at the end of the loop!
    //
    if ( (a_line.substr(0, ws*2) == bos) && ( sentence_wordcount> 0) ) {
      double avg_ent  = sum_logprob / (double)sentence_wordcount;
      double avg_wlp  = sum_wlp / (double)sentence_wordcount; 
      double avg_pplx = pow( 2, -avg_ent ); 
      file_out1 << sentence_count << " "
		<< sentence_wordcount << " "
		<< sum_logprob << " "
		<< avg_pplx << " "
		<< avg_wlp << " ";

      double sum_avg_diff = 0;
      std::string tmp_output;
      std::vector<double>::iterator vi;
      vi = w_pplx.begin();
      //file_out1 << "[ ";
      tmp_output = " [ ";
      while ( vi != w_pplx.end() ) {
	//file_out1 << *vi << ' ';
	tmp_output = tmp_output + to_str(*vi) + " ";
	sum_avg_diff += (*vi - avg_pplx) * (*vi - avg_pplx);
	vi++;
      }
      tmp_output += "]";
      //file_out1 << "] ";

      double std_dev = sqrt( sum_avg_diff / sentence_wordcount );
      //file_out1 << std_dev;
      file_out1 << std_dev << tmp_output;
      file_out1 << std::endl;
      sum_logprob = 0.0;
      sentence_wordcount = 0;
      ++sentence_count;
      w_pplx.clear();
    } // end bos

    ++sentence_wordcount;

    // Is the target in the lexicon? We could calculate a smoothed
    // value here if we load the .cnt file too...
    //
    std::map<std::string,int>::iterator wfi = wfreqs.find( target );
    bool   target_unknown = false;
    bool   target_in_dist = false;
    double target_lexfreq = 0.0;// should be double because smoothing
    double target_lexprob = 0.0;
    if ( wfi == wfreqs.end() ) {
      target_unknown = true;
    } else {
      target_lexfreq =  (int)(*wfi).second; // Take lexfreq, unless we smooth
      std::map<int,double>::iterator cfi = c_stars.find( target_lexfreq );
      if ( cfi != c_stars.end() ) { // We have a smoothed value, use it
	target_lexfreq = (double)(*cfi).second;
	//l.log( "Changed lexfreq to: " + to_str(target_lexfreq));
      }
      target_lexprob = (double)target_lexfreq / (double)total_count;
    }

    // What does Timbl think?
    //
    // We can also cache this to avoid calling Timbl for 
    // "_ _ xx" patterns, to avoid calling Timbl in the beginning
    // of every sentence and taking a lot of time.
    //
    //
    long us0 = clock_u_secs();
    tv = My_Experiment->Classify( a_line, vd );
    long us1 = clock_u_secs();
    //l.log("Timbl took: "+to_str(us1-us0, 6, ' ')+" usecs.");
    timbl_time += (us1-us0);

    std::string answer = tv->Name();
    if ( vd == NULL ) {
      l.log( "Classify( a_line, vd ) was null, skipping current line." );
      file_out << a_line << ' ' << answer << " ERROR" << std::endl;
      continue;
    } 

    size_t md  = My_Experiment->matchDepth();
    bool   mal = My_Experiment->matchedAtLeaf();
    //l.log( "md="+to_str(md)+", mal="+to_str(mal) );

    // Loop over distribution returned by Timbl.
    //
    // entropy over distribution: sum( p log(p) ). 
    //
    Timbl::ValueDistribution::dist_iterator it = vd->begin();
    int cnt = 0;
    int distr_count = 0;
    double smoothed_distr_count = 0.0;
    int target_freq = 0;
    int answer_freq = 0;
    double prob            = 0.0;
    double target_distprob = 0.0;
    double answer_prob     = 0.0;
    double entropy         = 0.0;
    std::vector<distr_elem> distr_vec;// see correct in levenshtein.
    cnt         = vd->size();
    distr_count = vd->totalSize();

    // Check cache/size/presence. Note it assumes that each size
    // only occurs once...
    //
    int cache_idx = -1;
    cached_distr* cd = NULL;
    for ( int i = 0; i < cache_size; i++ ) {
      if ( distr_cache.at(i).cnt == cnt ) {
	if ( distr_cache.at(i).sum_freqs == distr_count ) {
	  cache_idx = i; // it's cached already!
	  cd = &distr_cache.at(i);
	  break;
	}
      }
    }
    if ( cache_idx == -1 ) { // It should be cached, if not present.
      if ( (cnt > cache_threshold) && (cnt > lowest_cache) ) {
	cd = &distr_cache.at( cache_size-1 ); // the lowest.
	l.log( "New cache: "+to_str(cnt)+" replacing: "+to_str(cd->cnt) );
	cd->cnt = cnt;
	cd->sum_freqs  = distr_count;
	(cd->distr_vec).clear();
      }
    }
    // cache_idx == -1 && cd == null: not cached, don't want to (small distr).
    // cache_idx == -1 && cd != null: not cached, want to cache it.
    // cache_idx  > -1 && cd == null: impossible
    // cache_idx  > -1 && cd != null: cached, cd points to cache.
    // 0, 1, 2, 3
    //
    int cache_level = -1; // see above.

    if (cache_idx == -1) {
      if ( cd == NULL ) {
	cache_level = 0;
      } else {
	cache_level = 1; // want to cache
      }
    } else if (cache_idx != -1) {
      if ( cd != NULL ) {
	cache_level = 3;
      } else {
	cache_level = 2;// play mission impossible theme
      }
    }

    // ----

    if ( cache_level == 3 ) { // Use the cache, Luke.
      std::map<std::string,int>::iterator wfi = (cd->freqs).find( target );
      if ( wfi != (cd->freqs).end() ) {
	target_freq = (long)(*wfi).second;
	target_in_dist = true;
      }
      entropy = cd->entropy;
      distr_vec = cd->distr_vec; // the [distr] we print
    }
    if ( (cache_level == 1) || (cache_level == 0) ) { // go over Timbl distr.

      while ( it != vd->end() ) {
	//const Timbl::TargetValue *tv = it->second->Value();

	std::string tvs  = it->second->Value()->Name();
	double      wght = it->second->Weight(); // absolute frequency.
	
	if ( topn > 0 ) { // only save if we want to sort/print them later.
	  distr_elem  d;
	  d.name   = tvs;
	  d.freq   = wght;
	  d.s_freq = wght;
	  distr_vec.push_back( d );
	}
	
	if ( tvs == target ) { // The correct answer was in the distribution!
	  target_freq = wght;
	  target_in_dist = true;
	}

	// Save it in the cache?
	//
	if ( cache_level == 1 ) {
	  cd->freqs[tvs] = wght;
	}

	// Entropy of whole distr. Cache.
	//
	prob     = (double)wght / (double)distr_count;
	entropy -= ( prob * log2(prob) );
	
	++it;
      } // end loop distribution
      if ( cache_level == 1 ) {
	cd->entropy = entropy;
      }
    } // cache_level == 1 or 0


    // Counting correct guesses
    //
    if ( answer == target ) {
      ++correct;
    } else if ( (answer != target) && (target_in_dist == true) ) {
      ++correct_distr; 
    } else {
      ++wrong;
    }

    target_distprob = (double)target_freq / (double)distr_count;

    // If correct: if target in distr, we take that prob, else
    // the lexical prob.
    // Unknown words?
    //
    double w_prob  = 0.0;
    std::string info = "huh?";
    if ( target_freq > 0 ) { // Right answer was in distr.
      w_prob = target_distprob;
      info = "target_distprob";
    } else {
      if ( ! target_unknown ) { // Wrong, we take lex prob if known target
	w_prob = target_lexprob;
	info = "target_lexprob";
      } else {
	//
	// What to do here? We have an 'unknown' target, i.e. not in the
	// lexicon.
	//
	w_prob = p0;
	info = "P(new_particular)";
      }
    }

    // (ref. Antal's mail 21/11/08)
    // word level pplx: 2 ^ (-logprob(w)) 
    //
    // What we want: average word_lp and standard dev.
    //
    double logprob = log2( w_prob );
    double word_lp = pow( 2, -logprob );
    sum_logprob += logprob;
    sum_wlp     += word_lp;
    w_pplx.push_back( word_lp );

    // What do we want in the output file? Write the pattern and answer,
    // the logprob, followed by the entropy (of distr.), the size of the
    // distribution returned, and the top-10 (or less) of the distribution.
    //
    // #instance+target classification logprob entropy word_lp (dist.cnt [topn])
    //
    file_out << a_line << ' ' << answer << ' '
	     << logprob << ' ' /*<< info << ' '*/ << entropy << ' ';
    file_out << word_lp << ' ';

    if ( answer == target ) {
      file_out << "cg "; // correct guess
    } else if ( (answer != target) && (target_in_dist == true) ) {
      file_out << "cd "; // correct distr.
    } else {
      file_out << "ic "; // incorrect
    }

    // New in 1.10.0, the matchDepth and matchedAtLeaf
    //
    file_out << md << ' ' << mal << ' ';

    if ( topn > 0 ) { // we want a topn, sort and print them. (Cache them?)
      int cntr = topn;
      sort( distr_vec.begin(), distr_vec.end() ); // not when cached?
      std::vector<distr_elem>::iterator fi;
      fi = distr_vec.begin();
      file_out << cnt << " [ ";
      while ( (fi != distr_vec.end()) && (--cntr >= 0) ) { // cache only those?
	file_out << (*fi).name << ' ' << (*fi).freq << ' ';
	if ( cache_level == 1 ) {
	  distr_elem d;
	  d.name = (*fi).name;
	  d.freq = (*fi).freq;
	  (cd->distr_vec).push_back( d );
	}
	fi++;
      }
      file_out << "]";
    }

    file_out << std::endl;

    // End of sentence (sort of)
    //
    if ( (target == "</s>")
	 //|| (target == ".") || (target == "!") || (target == "?") 
	 ) {
      double avg_ent  = sum_logprob / (double)sentence_wordcount;
      double avg_wlp  = sum_wlp / (double)sentence_wordcount; 
      double avg_pplx = pow( 2, -avg_ent ); 
      file_out1 << sentence_count << " "
		<< sentence_wordcount << " "
		<< sum_logprob << " "
		<< avg_pplx << " "
		<< avg_wlp << " ";

      double sum_avg_diff = 0;
      std::string tmp_output;
      std::vector<double>::iterator vi;
      vi = w_pplx.begin();
      //file_out1 << "[ ";
      tmp_output = " [ ";
      while ( vi != w_pplx.end() ) {
	//file_out1 << *vi << ' ';
	tmp_output = tmp_output + to_str(*vi) + " ";
	sum_avg_diff += (*vi - avg_pplx) * (*vi - avg_pplx);
	vi++;
      }
      tmp_output += "]";
      //file_out1 << "] ";

      double std_dev = sqrt( sum_avg_diff / sentence_wordcount );
      //file_out1 << std_dev;
      file_out1 << std_dev << tmp_output;
      file_out1 << std::endl;
      sum_logprob = 0.0;
      sentence_wordcount = 0;
      ++sentence_count;
      w_pplx.clear();
    }

    // Find new lowest here. Overdreven om sort te gebruiken?
    //
    if ( cache_level == 1 ) {
      sort( distr_cache.begin(), distr_cache.end() );
      lowest_cache = distr_cache.at(cache_size-1).cnt;
      //}
    lowest_cache = 1000000; // hmmm.....
    for ( int i = 0; i < cache_size; i++ ) {
      if ( (distr_cache.at(i)).cnt < lowest_cache ) {
	lowest_cache = (distr_cache.at(i)).cnt;
      }
    }
    }

  } // while getline()

  if ( sentence_wordcount > 0 ) { // Left over (or all)
      double avg_ent  = sum_logprob / (double)sentence_wordcount;
      double avg_wlp  = sum_wlp / (double)sentence_wordcount; 
      double avg_pplx = pow( 2, -avg_ent ); 
      file_out1 << sentence_count << " "
		<< sentence_wordcount << " "
		<< sum_logprob << " "
		<< avg_pplx << " "
		<< avg_wlp << " ";

      double sum_avg_diff = 0;
      std::string tmp_output;
      std::vector<double>::iterator vi;
      vi = w_pplx.begin();
      //file_out1 << "[ ";
      tmp_output = " [ ";
      while ( vi != w_pplx.end() ) {
	//file_out1 << *vi << ' ';
	tmp_output = tmp_output + to_str(*vi) + " ";
	sum_avg_diff += (*vi - avg_pplx) * (*vi - avg_pplx);
	vi++;
      }
      tmp_output += "]";
      //file_out1 << "] ";

      double std_dev = sqrt( sum_avg_diff / sentence_wordcount );
      //file_out1 << std_dev;
      file_out1 << std_dev << tmp_output;
      file_out1 << std::endl;
      sum_logprob = 0.0;
      sentence_wordcount = 0;
      ++sentence_count;
      w_pplx.clear();
  }

  file_out1.close();
  file_out.close();
  file_in.close();

  l.log( "Correct:       " + to_str(correct) );
  l.log( "Correct Distr: " + to_str(correct_distr) );
  int correct_total = correct_distr+correct;
  l.log( "Correct Total: " + to_str(correct_total) );
  l.log( "Wrong:         " + to_str(wrong) );
  if ( sentence_wordcount > 0 ) {
    l.log( "Cor.tot/total: " + to_str(correct_total / (double)sentence_wordcount) );
    l.log( "Correct/total: " + to_str(correct / (double)sentence_wordcount) );
  }

  /*for ( int i = 0; i < cache_size; i++ ) {
    l.log( to_str(i)+"/"+to_str((distr_cache.at(i)).cnt) );
    }*/

  l.log("Timbl took: "+secs_to_str(timbl_time/1000000) );

  return 0;
}
#else
int pplx_simple( Logfile& l, Config& c ) {
  l.log( "No TIMBL support." );
  return -1;
}  
#endif

#ifdef TIMBL
// wopr -r multi -p kvs:ibases.kvs,filename:zin.04,lexicon:austen.txt.ng2.lex
//
int multi( Logfile& l, Config& c ) {
  l.log( "multi" );
  const std::string& filename         = c.get_value( "filename" );
  const std::string& lexicon_filename = c.get_value( "lexicon" );
  const std::string& counts_filename  = c.get_value( "counts" );
  const std::string& kvs_filename     = c.get_value( "kvs" );
  const std::string& timbl            = c.get_value( "timbl" );
  // PJB: Should be a context string, also l2r2 &c.
  int                ws               = stoi( c.get_value( "ws", "3" ));
  bool               to_lower         = stoi( c.get_value( "lc", "0" )) == 1;
  std::string        output_filename  = filename + ".px" + to_str(ws);
  std::string        pre_s            = c.get_value( "pre_s", "<s>" );
  std::string        suf_s            = c.get_value( "suf_s", "</s>" );
  int                skip             = 0;
  Timbl::TimblAPI   *My_Experiment;
  std::string        distrib;
  std::vector<std::string> distribution;
  std::string        result;
  double             distance;

  l.inc_prefix();
  l.log( "filename:   "+filename );
  l.log( "lexicon:    "+lexicon_filename );
  l.log( "counts:     "+counts_filename );
  l.log( "kvs:        "+kvs_filename );
  l.log( "timbl:      "+timbl );
  l.log( "ws:         "+to_str(ws) );
  l.log( "lowercase:  "+to_str(to_lower) );
  l.log( "OUTPUT:     "+output_filename );
  l.dec_prefix();

  std::ifstream file_in( filename.c_str() );
  if ( ! file_in ) {
    l.log( "ERROR: cannot load inputfile." );
    return -1;
  }
  std::ofstream file_out( output_filename.c_str(), std::ios::out );
  if ( ! file_out ) {
    l.log( "ERROR: cannot write output file." );
    return -1;
  }

  // Load lexicon. NB: hapaxed lexicon is different? Or add HAPAX entry?
  //
  int wfreq;
  unsigned long total_count = 0;
  unsigned long N_1 = 0; // Count for p0 estimate.
  std::map<std::string,int> wfreqs; // whole lexicon
  std::ifstream file_lexicon( lexicon_filename.c_str() );
  if ( ! file_lexicon ) {
    l.log( "NOTICE: cannot load lexicon file." );
    //return -1;
  } else {
    // Read the lexicon with word frequencies.
    // We need a hash with frequence - countOfFrequency, ffreqs.
    //
    l.log( "Reading lexicon." );
    std::string a_word;
    while ( file_lexicon >> a_word >> wfreq ) {
      wfreqs[a_word] = wfreq;
      total_count += wfreq;
      if ( wfreq == 1 ) {
	++N_1;
      }
    }
    file_lexicon.close();
    l.log( "Read lexicon (total_count="+to_str(total_count)+")." );
  }

  // If we want smoothed counts, we need this file...
  // Make mapping <int, double> from c to c* ?
  //
  std::map<int,double> c_stars;
  int Nc0;
  double Nc1; // this is c*
  int count;
  std::ifstream file_counts( counts_filename.c_str() );
  if ( ! file_counts ) {
    l.log( "NOTICE: cannot read counts file, no smoothing will be applied." ); 
  } else {
    while( file_counts >> count >> Nc0 >> Nc1 ) {
      c_stars[count] = Nc1;
    }
    file_counts.close();
  }

  // The P(new_word) according to GoodTuring-3.pdf
  // We need the filename.cnt for this, because we really need to
  // discount the rest if we assign probability to the unseen words.
  //
  // We need to esitmate the total number of unseen words. Same as
  // vocab, i.e assume we saw half? Ratio of N_1 to total_count?
  //
  // We need to load .cnt file as well...
  //
  double p0 = 0.00001; // Arbitrary low probability for unknown words.
  if ( total_count > 0 ) { // Better estimate if we have a lexicon
    p0 = (double)N_1 / ((double)total_count * total_count);
  }

  // read kvs
  //
  std::map<int, Classifier*> ws_classifier; // size -> classifier.
  std::map<int, Classifier*>::iterator wsci;
  std::vector<Classifier*> cls;
  std::vector<Classifier*>::iterator cli;
  if ( kvs_filename != "" ) {
    l.log( "Reading classifiers." );
    std::ifstream file_kvs( kvs_filename.c_str() );
    if ( ! file_kvs ) {
      l.log( "ERROR: cannot load kvs file." );
      return -1;
    }
    read_classifiers_from_file( file_kvs, cls );
    l.log( to_str((int)cls.size()) );
    file_kvs.close();
    for ( cli = cls.begin(); cli != cls.end(); cli++ ) {
      l.log( (*cli)->id );
      (*cli)->init();
      int c_ws = (*cli)->get_ws();
      ws_classifier[c_ws] = *cli;
    }
    l.log( "Read classifiers." );
  }

  std::vector<std::string>::iterator vi;
  std::ostream_iterator<std::string> output( file_out, " " );

  std::string a_line;
  std::vector<std::string> results;
  std::vector<std::string> targets;
  std::vector<std::string>::iterator ri;
  const Timbl::ValueDistribution *vd;
  const Timbl::TargetValue *tv;
  std::vector<Multi*> multivec( 20 );
  std::vector<std::string> words;

  while( std::getline( file_in, a_line )) {

    if ( to_lower ) {
      std::transform(a_line.begin(),a_line.end(),a_line.begin(),tolower); 
    }

    // How long is the sentence? initialise Multi.h in advance,
    // one per word in the sentence?
    // Initialise correct target as well...
    //
    int multi_idx = 0;
    multivec.clear();
    words.clear();
    Tokenize( a_line, words, ' ' );
    int sentence_size = words.size();
    for ( int i = 0; i < sentence_size; i++ ) {
      multivec[i] = new Multi("");
      multivec[i]->set_target( words[i] );
    }

    // We loop over classifiers, vote which result we take.
    //
    for ( cli = cls.begin(); cli != cls.end(); cli++ ) {

      Classifier *classifier = *cli;
      l.log( "Classifier: " + (*cli)->id );
      int win_s = (*cli)->get_ws();
      //l.log( "win_s="+to_str(win_s) );

      Timbl::TimblAPI *timbl = classifier->get_exp();

      //      pattern target  lc    rc  backoff
      window( a_line, a_line, win_s, 0, false, results ); 

      // For each classifier, make data and run. We need to specify how to
      // make data. We have the ws parameter in the Classifier.
      //
      // If we get different size patterns (first 1, then 2, then 3, 3, 3) we
      // can choose he right classifier based on the size of the pattern.
      // So maybe we need yet-another-window function which does that
      // (essentially patterns without the _ markers).
      //
      // Add a function to the kvs file to make data?
      //
      // We need a data structure to gather all the results and probability
      // values... (classifier, classification, distr?, prob....)
      //
      multi_idx = 0;
      for ( ri = results.begin(); ri != results.end(); ri++ ) {
	std::string cl = *ri;
	file_out << cl << std::endl;

	tv = timbl->Classify( cl, vd );
	std::string answer = tv->Name();

	int cnt = vd->size();
	int distr_count = vd->totalSize();

	l.log( to_str(multi_idx) + ": " + cl + "/" + answer + " "+to_str(cnt) );

	multivec[multi_idx]->add_string( answer );
	multivec[multi_idx]->add_answer( classifier, answer );

	++multi_idx;
      } // results
      results.clear();
    } // classifiers

    for ( int i = 0; i < sentence_size; i++ ) {
      //l.log( words[i] + ": " + multivec[i]->get_answer() );
      l.log( multivec[i]->get_answers() );
    }
  }

  file_out.close();
  file_in.close();

  c.add_kv( "filename", output_filename );
  l.log( "SET filename to "+output_filename );
  return 0;
}
#else
int multi( Logfile& l, Config& c ) {
  l.log( "Timbl support not built in." );  
  return -1;
}
#endif

// Fill a keyword value hash
// What we want:
//
// classifier:one
// one:ibasefile:something.ws3.ibase
// one:ws:3
// one:foo:bar
// classifier:two
// ...
//
int read_kv_from_file( std::ifstream& file,
		       std::map<std::string, std::string>& res )  {
  std::string a_line;
  while( std::getline( file, a_line )) {
    if ( a_line.length() == 0 ) {
      continue;
    }
    int pos = a_line.find( ':', 0 );
    if ( pos != std::string::npos ) {
      std::string lhs = trim(a_line.substr( 0, pos ));
      std::string rhs = trim(a_line.substr( pos+1 ));
      res[lhs] = rhs;
    }
  }
  
  return 0;
}

int read_classifiers_from_file( std::ifstream& file,
				std::vector<Classifier*>& cl )  {
  std::string a_line;
  Classifier* c = NULL;
  while( std::getline( file, a_line )) {
    if ( a_line.length() == 0 ) {
      continue;
    }
    int pos = a_line.find( ':', 0 );
    if ( pos != std::string::npos ) {
      std::string lhs = trim(a_line.substr( 0, pos ));
      std::string rhs = trim(a_line.substr( pos+1 ));

      // Create a new one. If c != NULL, we store it in the cl vector.
      //
      if ( lhs == "classifier" ) {
	if ( c != NULL ) {
	  //store
	  cl.push_back( c );
	  c = NULL;
	}
	c = new Classifier( rhs );
      } else if ( lhs == "ibasefile" ) {
	// store this ibasefile
	if ( c != NULL ) {
	  c->set_ibasefile( rhs );
	}
      } else if ( lhs == "ws" ) {
	// store this window size
	if ( c != NULL) {
	    c->set_ws( stoi(rhs) );
	}
      } else if ( lhs == "timbl" ) {
	// e.g: "-a1 +vdb +D +G"
	if ( c != NULL) {
	    c->set_timbl( rhs );
	}
      }
    }
  }
  if ( c != NULL ) {
    cl.push_back( c );
  }
  
  return 0;
}

bool file_exists( Logfile& l, Config& c, const std::string& fn ) {
  struct stat file_info;
  int stat_res; 

  // We check if overwrite is "on", then we just return false.
  //
  if ( c.get_value( "overwrite", "0" ) == "1" ) {
    return false;
  }

  stat_res = stat( fn.c_str(), &file_info );
  if( stat_res == 0 ) {
    return true;
  }

  return false;
}
