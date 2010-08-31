// ---------------------------------------------------------------------------
// $Id$
// ---------------------------------------------------------------------------

/*****************************************************************************
 * Copyright 2007 - 2010 Peter Berck                                         *
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

#include "MersenneTwister.h"

// Socket stuff

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

// ----

#ifdef TIMBL
#include "timbl/TimblAPI.h"
#endif

#ifdef TIMBLSERVER
#include "SocketBasics.h"
#endif


/*
durian:wopr pberck$ ./wopr -r generate -p start:"Mexico has not",ibasefile:test/reuters.martin.tok.1000.ws3.ibase,timbl:"-a1 +D",len:10
*/
#ifdef TIMBL
int generate( Logfile& l, Config& c ) {
  l.log( "generate" );
  const std::string& start            = c.get_value( "start", "" );
  const std::string  filename         = c.get_value( "filename", to_str(getpid()) );
  const std::string& ibasefile        = c.get_value( "ibasefile" );
  const std::string& timbl            = c.get_value( "timbl" );
  const std::string  end              = c.get_value( "end", "." );
  int                ws               = stoi( c.get_value( "ws", "3" ));
  int                mode             = stoi( c.get_value( "mode", "1" ));
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
  l.log( "mode:       "+to_str(mode) );
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

  MTRand mtrand;

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
      if ( ! tv ) {
	l.log( "ERROR: Timbl returned a classification error, aborting." );
	break;
      }
      std::string answer = "";// tv->Name();
      cnt = vd->size();
      distr_count = vd->totalSize();

      if ( cnt == 1 ) {
	++no_choice;
      }
      total_choices += cnt;

      int rnd_idx;
      if ( mode == 0 ) {
	rnd_idx = mtrand.randInt( cnt-1 ); // % 3; // Extra "% max" ?
      } else {
	rnd_idx = mtrand.randInt( distr_count-1 ); //NEW
      }
      //l.log( to_str(rnd_idx)+"/"+to_str(cnt) );
      unsigned long sum = 0;

      // Take (top) answer, or choose something from the
      // distribution.
      //
      it = vd->begin();
      if ( mode == 0 ) {
	for ( int i = 0; i < rnd_idx; i++ ) {
	  ++it;
	}
      } else {
	for ( int i = 0; i < rnd_idx; i++ ) {
	  sum += it->second->Weight();
	  if ( sum > rnd_idx ) {
	    break;
	  }
	  ++it;
	}
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

      // Stop when "end" character.
      //
      std::string::size_type pos = end.find( tvs, 0 );
      if ( pos != std::string::npos ) {
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
			  std::string end,
			  Timbl::TimblAPI* My_Experiment ) {

  std::string result = "";
  std::vector<std::string>::iterator ri;
  const Timbl::ValueDistribution *vd;
  const Timbl::TargetValue *tv;
  std::vector<std::string> words;
  Timbl::ValueDistribution::dist_iterator it;
  int cnt;

  MTRand mtrand;

  words.clear();
  Tokenize( a_line, words, ' ' ); // if less than ws, add to a_line

  while ( --len >= 0 ) {
    a_line = a_line + " _";
    tv = My_Experiment->Classify( a_line, vd );
    if ( ! tv ) {
      break;
    }
    std::string answer = "";// tv->Name();
    cnt = vd->size();
    
    int rnd_idx = mtrand.randInt( cnt -1 );
    
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

    std::string::size_type pos = end.find( answer, 0 );
    if ( pos != std::string::npos ) {
      len = 0;
    }
    
  }

  return result;
  
}

// returns one sentence of length len.
//
std::string generate_xml( Config& c, std::string& a_line, int len, int ws,
			  std::string end, std::string id,
			  Timbl::TimblAPI* My_Experiment ) {

  std::vector<std::string>::iterator ri;
  const Timbl::ValueDistribution *vd;
  const Timbl::TargetValue *tv;
  std::vector<std::string> words;
  Timbl::ValueDistribution::dist_iterator it;
  int cnt;

  MTRand mtrand;

  a_line = trim( a_line, " \n\r" );
  words.clear();
  Tokenize( a_line, words, ' ' ); // if less than ws, add to a_line

  // Check size of initial a_line, adjust if necessary (to ws).
  //
  int wcount = words.size();
  if ( wcount < ws ) {
    std::string sos = "_ _ _ _ _ _ _ _";
    a_line = sos.substr(0, (ws-wcount)*2) + a_line;
  } else {
    a_line = "";
    for ( int i = wcount-ws; i < wcount; i++ ) {
      a_line = a_line + words.at(i) + " ";
    }
  }
  //std::cerr << "(" << a_line << ")" << std::endl;

  words.clear();
  Tokenize( a_line, words, ' ' ); 

  std::string result = "<sentence id=\""+id+"\">";
  int idx = 0;
  std::string tmp_res = "";

  for ( int i = 0; i < ws; i++ ) {
    if ( words.at(i) != "_" ) {
      result = result + "<word id=\""+to_str(idx)+"\" cnt=\"0\""
	+ " freq=\"0\" sumfreq=\"0\">";
      result = result + "<![CDATA[" + words.at(i) + "]]>";
      result = result + "</word>\n";

      tmp_res = tmp_res + words.at(i) + " ";
      ++idx;
    }
  }

  int mode = 1; // 0 is old

  while ( --len >= 0 ) {
    a_line = a_line + " ?";

    //std::cout << "Timbl(" << a_line << ")" << std::endl;

    tv = My_Experiment->Classify( a_line, vd );
    if ( ! tv ) {
      break;
    }
    std::string answer = "";// tv->Name();
    cnt = vd->size();
    int distr_count = vd->totalSize();

    int rnd_idx;
    if ( mode == 0 ) {
      rnd_idx = mtrand.randInt( cnt-1 ); //  % 3;
    } else {
      rnd_idx = mtrand.randInt( distr_count-1 );
    }
    unsigned long sum = 0;

    // Take (top) answer, or choose something from the
    // distribution.
    //
    it = vd->begin();
    if ( mode == 0 ) {
      for ( int i = 0; i < rnd_idx; i++ ) {
	std::string tvs  = it->second->Value()->Name();
	++it;
      }
    } else {
      for ( int i = 0; i < rnd_idx; i++ ) {
        sum += it->second->Weight();
        if ( sum > rnd_idx ) {
          break;
        }
        ++it;
      }
    }
    std::string tvs  = it->second->Value()->Name();
    double      wght = it->second->Weight();
    answer = tvs;

    tmp_res = tmp_res + answer + "/" + to_str(cnt)+"/"+to_str(distr_count)+" ";

    result = result + "<word id=\""+to_str(idx)+"\" cnt=\""+to_str(cnt)+"\""
      + " freq=\""+to_str(wght)+"\" sumfreq=\""+to_str(distr_count)+"\">";
    result = result + "<![CDATA[" + answer + "]]>";
    result = result + "</word>\n";

    ++idx,

    // shift/add/repeat
    //
    copy( words.begin()+1, words.end(), words.begin() );
    words.at(ws-1) = answer;
    
    a_line = "";
    for ( int i = 0; i < ws; i++ ) {
      a_line = a_line + words[i] + " ";
    }  

    std::string::size_type pos = end.find( answer, 0 );
    if ( pos != std::string::npos ) {
      len = 0;
    }

  }

  std::cout << tmp_res << std::endl;
  tmp_res = "";

  result = result + "</sentence>";

  return result;  
}
#endif

// Needs both TIMBL and TIMBLSERVER

#if defined(TIMBLSERVER) && defined(TIMBL)
int generate_server( Logfile& l, Config& c ) {
  l.log( "new_generate_server" );
  const std::string  port             = c.get_value( "port", "1988" );
  std::string        start            = c.get_value( "start", "" );
  const std::string& ibasefile        = c.get_value( "ibasefile" );
  const std::string& timbl            = c.get_value( "timbl" );
  const std::string& end              = c.get_value( "end", "" );
  int                lc               = stoi( c.get_value( "lc", "2" ));
  int                rc               = stoi( c.get_value( "rc", "0" ));
  int                len              = stoi( c.get_value( "len", "50" ) );
  const int          verbose          = stoi( c.get_value( "verbose", "0" ));
  int                n                = stoi( c.get_value( "n", "10" ) );
  Timbl::TimblAPI   *My_Experiment;

  int ws = lc+rc;
  MTRand mtrand;

  l.inc_prefix();
  l.log( "port:       "+port );
  l.log( "ibasefile:  "+ibasefile );
  l.log( "timbl:      "+timbl );
  l.log( "lc:         "+to_str(lc) );
  l.log( "rc:         "+to_str(rc) );
  l.log( "end:        "+end ); // end marker of sentences
  l.log( "n:          "+to_str(n) ); // number of sentences
  l.log( "len:        "+to_str(len) ); // max length of sentences
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

  // ----

  Sockets::ServerSocket server;

  if ( ! server.connect( port )) {
    l.log( "ERROR: cannot start server: "+server.getMessage() );
    return 1;
  }
  if ( ! server.listen(  ) < 0 ) {
    l.log( "ERROR: cannot listen. ");
    return 1;
  };

  l.log( "Starting server..." );

  // ----

  std::string a_line;
  std::string result;
  std::string buf;

  while ( true ) { 

    Sockets::ServerSocket *newSock = new Sockets::ServerSocket();
    if ( !server.accept( *newSock ) ) {
      if( errno == EINTR ) {
	continue;
      } else {
	l.log( "ERROR: " + server.getMessage() );
	return 1;
      }
    }
    if ( verbose > 0 ) {
      l.log( "Connection " + to_str(newSock->getSockId()) + "/"
	     + std::string(newSock->getClientName()) );
    }

    //newSock->write( "Greetings, earthling.\n" );

    newSock->read( buf );
    buf = trim( buf, " \n\r" );
    std::string tmp_buf = str_clean( buf );
    tmp_buf = trim( tmp_buf, " \n\r" );
    
    start = tmp_buf;
    l.log( "start="+start );
    
    // INfo about wopr
    std::string info = "<info>";
    info += "ibasefile:"+ibasefile+",lc:"+to_str(lc)+",rc:"+to_str(rc);
    info += "</info>";
    newSock->write( info );
    
    int n1 = n;
    while ( --n1 >= 0 ) {
      a_line = start;
      
      //int len1 = mtrand.randInt( len )+1;
      
      std::string foo = generate_xml( c, a_line, len, ws, end, to_str(n1),
				      My_Experiment );
      
      //foo = "<data><![CDATA[" + foo + "]]></data>";
      newSock->write( foo );
    } // --n
    
    l.log( "ready." );

    delete newSock;

  } // while true

  return 0;
}
#else
int generate_server( Logfile& l, Config& c ) {
  l.log( "No TIMBL/SERVER support." );
  return -1;
}  
#endif
