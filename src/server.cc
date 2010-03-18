// ---------------------------------------------------------------------------
// $Id$
// ---------------------------------------------------------------------------

/*****************************************************************************
 * Copyright 2007, 2010 Peter Berck                                          *
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

/*
  pberck@pberck-desktop:~/prog/c/wpred$ ./wopr -s etc/exp01_train -p lines:8888,filename:reuters.martin.tok

  pberck@pberck-desktop:~/prog/c/wpred$ ./wopr -r server2 -p ibasefile:reuters.martin.tok.cl8888.ws7.hpx1.ibase,timbl:"-a1 +D"

  ->  interest continues to rise in the east
*/

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

#include <unistd.h>
#include <stdio.h>

#include <sys/msg.h>

#include "qlog.h"
#include "util.h"
#include "Config.h"
#include "runrunrun.h"
#include "watcher.h"
#include "server.h"

#ifdef TIMBL
#include "timbl/TimblAPI.h"
#endif

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

#define BACKLOG 5     // how many pending connections queue will hold
#define MAXDATASIZE 2048 // max number of bytes we can get at once 

void sigchld_handler( int s ) {
  while(waitpid(-1, NULL, WNOHANG) > 0);
}

// ./wopr --run server2 -p ibasefile:tekst.txt.l1000.ws7.ibase,timbl:"-a1 +D"
// pberck@lg0216:/x/work/wpred$ ./wopr -r server2 -p ibasefile:reuters.martin.tok.fl1000.ws7.hpx1.ibase,lexicon:reuters.martin.tok.fl10000.lex,timbl:"-a1 +vdb +D",unk_prob:0.00000001
//
/*
test$ ../wopr -r server2 -p ibasefile:reuters.martin.tok.1000.ws3.ibase,lexicon:reuters.martin.tok.1000.lex,timbl:"-a1 +vdb +D",unk_prob:0.00000001,ws:3,sentence:0
*/
#ifdef TIMBL
int server2(Logfile& l, Config& c) {
  l.log( "server2" );
  
  const std::string& timbl  = c.get_value( "timbl" );
  const int port = stoi( c.get_value( "port", "1984" ));
  const int precision = stoi( c.get_value( "precision", "10" )); // c++ output
                                                                // precision
  const int output = stoi( c.get_value( "output", "1" ));
  const int hpx_t  = stoi( c.get_value( "hpx_t", "0" )); // Hapax target
  const std::string& lexicon_filename = c.get_value( "lexicon" );
  int ws = stoi( c.get_value( "ws", "7" ));
  int hapax = stoi( c.get_value( "hpx", "0" ));
  std::string hpx_sym = c.get_value("hpx_sym", "<unk>"); // not used, hardcoded in hapax_line
  double unk_prob = stod( c.get_value( "unk_prob", "0.000001" ));
  const int sentence = stoi( c.get_value( "sentence", "1" ));

  l.inc_prefix();
  l.log( "ibasefile: " + c.get_value("ibasefile") );
  l.log( "port:      "+to_str(port) );
  l.log( "timbl:     "+timbl );
  l.log( "ws:        "+to_str(ws) );
  l.log( "hpx:       "+to_str(hapax) );
  l.log( "hpx_t:     "+to_str(hpx_t) );
  l.log( "precision: "+to_str(precision) );
  l.log( "output:    "+to_str(output) );
  l.log( "lexicon:   " + lexicon_filename );
  l.log( "unknown p: " + to_str(unk_prob) );
  l.log( "sentence:  " + to_str(sentence) );
  l.dec_prefix();

  create_watcher( c );

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
  while( file_lexicon >> a_word >> wfreq ) {
    if ( wfreq > hapax ) {
      wfreqs[a_word] = wfreq;
      total_count += wfreq; // here?
    }
  }
  file_lexicon.close();
  l.log( "Read lexicon (total_count="+to_str(total_count)+")." );

  // To fool the hapax_line2 function:
  // (this affects probability)
  //
  //wfreqs["_"] = hapax + 1;
  //wfreqs["HAPAX"] = hapax + 1;

  // Init message queue.
  //
  key_t wopr_id = (key_t)stoi( c.get_value( "id", "88" ));
  int   msqid   = msgget( wopr_id, 0666 | IPC_CREAT );

  std::string distrib;
  std::vector<std::string> distribution;
  std::string result;
  double distance;
  double total_prplx = 0.0;
  const Timbl::ValueDistribution *vd;
  const Timbl::TargetValue *tv;
  
  try {
    Timbl::TimblAPI *My_Experiment = new Timbl::TimblAPI( timbl );
    (void)My_Experiment->GetInstanceBase( c.get_value( "ibasefile" ));

    //My_Experiment->StartServer( 8888, 1 );
    // here we start socket server and wait/process.

    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct sockaddr_in my_addr;    // my address information
    struct sockaddr_in their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    int numbytes;  
    char buf[MAXDATASIZE];

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      perror("socket");
      exit(1);
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      perror("setsockopt");
      exit(1);
    }

    int ka = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &ka, sizeof(int)) == -1 ) {
      perror("setsockopt");
      exit(1);
    }
      
    my_addr.sin_family = AF_INET;         // host byte order
    my_addr.sin_port = htons(port);       // short, network byte order
    my_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
    memset(my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);
    
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof my_addr) == -1) {
      perror("bind");
      exit(1);
    }
    
    if (listen(sockfd, BACKLOG) == -1) {
      perror("listen");
      exit(1);
    }
    
    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if ( sigaction( SIGCHLD, &sa, NULL ) == -1 ) {
      perror( "sigaction" );
      exit( 1 );
    }

    // loop
    int  verbose = 0;
    struct wopr_msgbuf wmb = {2, verbose, 8}; // children get copy.

    while ( c.get_status() != 0 ) {  // main accept() loop
      sin_size = sizeof their_addr;

      if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr,
			   &sin_size)) == -1) {
	perror("accept");
	continue;
      }
      l.log( "Connection from: " +std::string(inet_ntoa(their_addr.sin_addr)));

      // Measure processing time.
      //
      long f1 = l.clock_mu_secs();

      // socket_file before fork?

      if ( c.get_status() && ( ! fork() )) { // this is the child process
	close(sockfd); // child doesn't need the listener.	

	bool connection_open = true;
	while ( connection_open ) {

	long f000 = l.clock_mu_secs();
	
	if ((numbytes=recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1) {
	  perror("recv");
	  exit(1);
	}
	buf[numbytes] = '\0';
	//printf("Received: %s",buf);

	long f001 = l.clock_mu_secs();
	long diff001 = f001 - f000;
	l.log( "Waiting & Receiving took (mu-sec): " + to_str(diff001) );

	std::string tmp_buf = str_clean( buf );
	tmp_buf = trim( tmp_buf, " \n\r" );

	if ( (tmp_buf == "") || (tmp_buf == "_CLOSE_" ) ) {
	  connection_open = false;
	  break;
	}

	// ----
	// here we got a line to test. Now what if this were a filename?
	// Start looping over file here.
	// ----

	if ( tmp_buf.substr( 0, 5 ) == "file:" ) {

	  l.log( "NOTICE: file mode in socket!" );
	  int pos = tmp_buf.find( ':', 0 );
	  if ( pos != std::string::npos ) {
	    std::string lhs = trim(tmp_buf.substr( 0, pos ));
	    std::string rhs = trim(tmp_buf.substr( pos+1 ));
	    l.log( "Filename: " + rhs );
	    socket_file( l, c, My_Experiment, rhs, wfreqs, total_count);
	    std::string p = "ready.";
	    if ( send( new_fd, p.c_str(), p.length(), 0 ) == -1 ) {
	      perror("send");
	    }
	  } else {
	    l.log( "ERROR: cannot parse filename." );
	  }
	}

	//
	// ----

	// Hapax.
	//
	std::string classify_line;
	if ( hapax > 0 ) {
	  hapax_line( tmp_buf, wfreqs, hapax, 1, classify_line );
	} else {
	  classify_line = tmp_buf;
	}

	// Start & end symbols. Set sentence:0 to enable.
	//
	std::string ssym = "<s>";
	std::string esym = "</s>";
	if ( sentence == 1 ) {
	  classify_line = ssym + " " + classify_line + " " + esym; 
	}
	l.log( "|" + classify_line + "|" );

	std::vector<std::string> results;
	std::vector<std::string> targets;
	std::vector<std::string>::iterator ri;

	// The targets are in a seperate array, which is (in this case)
	// the same as the input line.
	//
	std::string t;
	if ( (hapax > 0) && (hpx_t == 1) ) { // If hpx_t is one, we HPX targets
	  hapax_line( tmp_buf, wfreqs, hapax, 1, t );
	} else {
	  t = tmp_buf;
	}
	if ( sentence == 1 ) {
	  t = ssym + " " + t + " " + esym; 
	}
	Tokenize( t, targets, ' ' );
	std::vector<std::string>::iterator ti = targets.begin();

	double total_logprob = 0.0; // Sum of log2(p)

	// The JSON reply to the client (nc or timbl.php).
	//
	std::string q;
	std::string json;
	std::string js_cl;
	std::string js_p;
	if ( output == 1 ) { // 1 is JSON output
	  q     = "\'";
	  json  = "{"+q+"results"+q+":[";
	  js_cl = q+"classifications"+q+":[";
	  js_p  = q+"probs"+q+":["; // what kind op probabilities
	}

	// results is an array with the line windowed.
	// The targets are in another array.
	//
	// WHERE DO WE HAPAX? After, or before we window?
	//
	window( classify_line, t, ws, 0, false, results );
	for ( ri = results.begin(); ri != results.end(); ri++ ) {

	  std::string cl = *ri;
	  
	  //l.log( cl ); // debug output

	  // Classify this buf.
	  //
	  // Update: If the last word in the context (the word before the
	  // target is unknown, we skip Timbl, and take the a priori
	  // word probability from the lexicon. This avoids Timbl
	  // failing on the top-node and returning a distribution
	  // containing everything.
	  //
	  std::string target = *ti; // was *ri

	  // Determine word before target. Look it up.
	  // If unknow, skip classification because it will only
	  // return the default distro (big).
	  //
	  std::vector<std::string> lcs; // left context, last word.
	  Tokenize( cl, lcs, ' ' );
	  std::string lc = lcs.at(ws-1);
	  std::map<std::string,int>::iterator wfi = wfreqs.find(lc);
	  bool lc_unknown = false;
	  /*
	  if ( ( lc != "_") && (wfi == wfreqs.end()) ) {
	    lc_unknown = true;
	  }
	  */

	  if ( lc_unknown == false ) {
	    //My_Experiment->Classify( cl, result, distrib, distance );
	    tv = My_Experiment->Classify( cl, vd, distance );
	    result = tv->Name();
	  } else {
	    //
	    // Set some dummy values so we can fall through the rest
	    // of the code.
	    //
	    //l.log( "Skipping Timbl for unknown context word:" + lc );
	    result   = "__SKIPPED__"; // make sure this is not in lexicon...
	    distrib  = "{}";
	    distance = 10.0;
	  }

	  // Grok the distribution returned by Timbl.
	  //
	  std::map<std::string, double> res;
	  parse_distribution2( vd, res ); // was parse_distribution(...)

	  // Start calculating.
	  //
	  double prob = 0.0;
	  if ( target == result ) {
	    // we got it, no confusion.
	    // Count right answers?
	  }
	  //
	  // Look for the target word in the distribution returned by Timbl.
	  // If it wasn't, look it up in the lexicon.
	  //   If in lexicon, take lexicon prob.
	  //   If not in lexicon, unknown, take default low prob. SMOOTHING!
	  // If it was in distribution
	  //    Take prob. in distribution.
	  //
	  std::map<std::string,double>::iterator foo = res.find(target);
	  if ( foo == res.end() ) { // not found. Problem for log.
	    //
	    // Fall back 1: look up in dictionary.
	    //
	    std::map<std::string,int>::iterator wfi = wfreqs.find(target);
	    if ( wfi == wfreqs.end() ) { // not found.
	      if ( output == 1) {
		js_p = js_p + "{" + q + json_safe(target) + q + ":" 
		  + q + "unk" + q + "}";
	      }
	    } else {
	      // Found, take p(word in lexicon)
	      //
	      prob = (int)(*wfi).second / (double)total_count ;
	      if ( output == 1) {
		js_p = js_p + "{" + q + json_safe(target) + q + ":" 
		  + q + "lex" + q + "}";
	      }
	    }
	  } else {
	    //
	    // Our target was in the distribution returned by Timbl.
	    // Take the probability of word in distribution.
	    //
	    prob = (*foo).second;
	    if ( output == 1 ) {
	      js_p = js_p + "{" + q + json_safe(target) + q + ":" 
		+ q + "dist" + q + "}";
	    }
	  }

	  // Good Turing smoothing oid. implementeren :)
	  //
	  if ( prob == 0.0 ) {
	    prob = unk_prob; // KLUDGE!!! for testing.
	  }
	  total_logprob += log2( prob );

	  if ( verbose == 1 ) {
	    l.log( "[" + cl + "] = " + result );
	  }

	  /* Takes time!
	  std::cout << "[" << cl << "] = " << result 
		    << "/" << distance << " "
		    << res.size() << " " 
		    << "p("+target+")=" << prob
		    << std::endl;
	  //std::cout << distrib << std::endl;
	  */

	  if ( output == 1 ) {
	    json = json + "{" + q + json_safe(result) + q + ":" 
	      + q + to_str( prob, precision ) + q + "}";
	    
	    js_cl = js_cl + "{" + q + json_safe(target) + q + ":" 
	      + q + json_safe(result) + q + "}";
	    
	    if ( ri != results.end()-1 ) { // add a comma except for the last
	      json  = json  + ",";
	      js_cl = js_cl + ",";
	      js_p  = js_p  + ",";
	    }
	  }
	  result = result + " " + to_str(prob, precision) + "\n"; // not used?

	  //if (send(new_fd, result.c_str(), result.length(), 0) == -1)
	  //perror("send");

	  ti++;
	} // end sentence

	long f2 = l.clock_mu_secs();
	long diff_mu_secs = f2 - f001;
	l.log( "Processing took (mu-sec): " + to_str(diff_mu_secs) );

	total_logprob = - total_logprob;
	double avg_logprob = total_logprob / (double)results.size();
	double total_prplx = pow( 2, avg_logprob );
	l.log( "total_prplx = " + to_str(total_prplx) );
	result = to_str(total_prplx, precision) + "\n";

	if ( output == 1 ) {
	  json = json + "]," + q + "prplx" + q + ":" 
	    + q + to_str(total_prplx, precision) + q;
	  
	  js_cl = js_cl + "]";
	  js_p = js_p + "]";
	  
	  json = json + "," + q + "ibasefile" + q + ":"
	    + q + c.get_value( "ibasefile" ) + q;
	  
	  json = json + "," + q + "lexicon" + q + ":"
	    + q + lexicon_filename + q;
	  
	  json = json + "," + q + "total_count" + q + ":"
	    + q + to_str( total_count ) + q;
	  
	  json = json + "," + q + "status" + q + ":"
	    + q + "ok" + q;
	  
	  json = json + "," + q + "microseconds" + q + ":"
	    + q + to_str(diff_mu_secs) + q;
	  
	  json = json + "," + js_cl; 
	  json = json + "," + js_p; 
	  
	  json = json + " }\n";
	  
	  // Should send extra info from c?

	  if ( send( new_fd, json.c_str(), json.length(), 0 ) == -1 ) {
	    perror("send");
	  }
	}
	
	if ( output == 0 ) { // 0 is perplexity only
	  std::string p = to_str(total_prplx) + "\n";
	  if ( send( new_fd, p.c_str(), p.length(), 0 ) == -1 ) {
	    perror("send");
	  }
	}
	
	//close(new_fd);
	
	long f3 = l.clock_mu_secs();
	long diff2_mu_secs = f3 - f2;
	l.log( "Sending over socket took (mu-sec): " + to_str(diff2_mu_secs) );
	} // connection_open
	exit(0);
      } // fork
      close( new_fd );  
    }
  }
  catch ( const std::exception& e ) {
    l.log( "ERROR: exception caught." );
    return -1;
  }

  msgctl( msqid, IPC_RMID, NULL ); // Kill queue

  return 0;
}
#else
int server2( Logfile& l, Config& c ) {
  l.log( "No TIMBL support." );
  return -1;
}
#endif

std::string json_safe( const std::string& s ) {
  std::string json_s = s;
  std::string::size_type idx = 0;
  /*
  while (true) {
    idx = s.find( "'", idx);
    if ( idx == string::npos )
      break;
    s.replace( idx, search.size(), "\\\'" );
    idx += 2;//replace.length();
  }
  */
  idx = json_s.find("'");
  if ( idx == std::string::npos ) {
    return s;
  }
  json_s.replace( json_s.find("'"), 1, "\\'" );
  return json_s;
}

std::string str_clean( const std::string& s ) {
  std::string clean;
  for ( int i = 0; i < s.length(); i++ ) {
    char c = s.at(i);
    if ( c < 32 ) {
      continue;// make it: c = 32, then fall through rest?
    }
    if ( (c == 32) && ( s.at(i+1) == 32) ) { // NB!
      //                      ^works because last never 32
      continue;
    }
    if ( c == '\\' ) {
      continue;
    }
    clean = clean + c;
  }
  return clean;
}

// tv = My_Experiment->Classify( *ri, vd );
//
#ifdef TIMBL
int parse_distribution2( const Timbl::ValueDistribution* vd,
			 std::map<std::string, double>& res ) {

  Timbl::ValueDistribution::dist_iterator it = vd->begin();
  int cnt = vd->size();
  int distr_count = vd->totalSize();
  
  while ( it != vd->end() ) {
    
    std::string tvs  = it->second->Value()->Name();
    double      wght = it->second->Weight();
    
    // Prob. of this item in distribution.
    //
    double prob = (double)wght / (double)distr_count;
    res[tvs] = prob;

    ++it;
  }

  return 0;
}
#endif

// Insert smoothed values here?
//
int parse_distribution( std::string dist, std::map<std::string, double>& res ) {

  std::vector<std::string> distribution;

  Tokenize( dist, distribution, ' ' );
  //
  // nu hebben we paren? Eerste { overslaan.
  //
  bool is_class = true;
  int  sum      = 0;
  int  d_size   = 0;
  int  target_f = 0;
  std::string a_class;

  for ( int i = 1; i < distribution.size(); i++ ) {
    std::string token = distribution.at(i);
    if ( (token == "{") || (token == "}")) {
      continue;
    }
    if ( is_class ) { // the class
      ++d_size;
      a_class = token;
    } else { // the frequency, absolute count.
      token = token.substr(0, token.length()-1);
      int i_token = stoi( token );
      if ( i_token > 0 ) {
	sum += i_token; // als +G, this is 0
	res[a_class] = stoi( token );
      } else {
	res[a_class] = stod( token );
      }
    }
    is_class = ( ! is_class );
  }

  // If we run with +G, we get sum == 0, we don't have to normalize in
  // this case.
  //
  if ( sum == 0 ) {
    return 0;
  }

  // Normalize.
  //
  for( std::map<std::string,double>::iterator iter = res.begin(); iter != res.end(); iter++ ) {
    //std::cout << (*iter).first << " " << (*iter).second << "\n";
    (*iter).second = (double)(*iter).second / (double)sum; 
    //std::cout << (*iter).first << " " << (*iter).second << "\n";
  }

  return 0;
}

// File from socket.
//
#ifdef TIMBL
int socket_file( Logfile& l, Config& c, Timbl::TimblAPI *My_Experiment,
		 const std::string& fn,
		 std::map<std::string,int>& wfreqs, 
		 unsigned long total_count) {

  int ws = stoi( c.get_value( "ws", "7" ));
  const std::string& timbl  = c.get_value( "timbl" );
  double unk_prob = stod( c.get_value( "unk_prob", "0.000001" ));
  const int precision = stoi( c.get_value( "precision", "10" )); // c++ output

  std::string fo = fn + ".wopr";
  std::string fr = fo + ".ready";

  std::ifstream file_in( fn.c_str() );
  if ( ! file_in ) {
    l.log( "ERROR: cannot load file." );
    return -1;
  }
  std::ofstream file_out( fo.c_str(), std::ios::out );
  if ( ! file_out ) {
    l.log( "ERROR: cannot write file." );
    return -1;
  }

  std::string classify_line;
  std::string tmp_buf;
  int hapax    = 0;
  int hpx_t    = 0;
  int sentence = 1; // Parameter
  int verbose  = 0; // Parameter
  std::string distrib;
  std::vector<std::string> distribution;
  std::string result;
  double distance;
  double total_prplx = 0.0;
  const Timbl::ValueDistribution *vd;
  const Timbl::TargetValue *tv;

  while( std::getline( file_in, tmp_buf )) {

    if ( hapax > 0 ) {
      hapax_line( tmp_buf, wfreqs, hapax, 1, classify_line );
    } else {
      classify_line = tmp_buf;
    }

    // Start & end symbols. Set sentence:0 to enable.
    //
    std::string ssym = "<s>";
    std::string esym = "</s>";
    if ( sentence == 1 ) {
      classify_line = ssym + " " + classify_line + " " + esym; 
    }
    l.log( "|" + classify_line + "|" );

    std::vector<std::string> results;
    std::vector<std::string> targets;
    std::vector<std::string>::iterator ri;

    // The targets are in a seperate array, which is (in this case)
    // the same as the input line.
    //
    std::string t;
    if ( (hapax > 0) && (hpx_t == 1) ) { // If hpx_t is one, we HPX targets
      hapax_line( tmp_buf, wfreqs, hapax, 1, t );
    } else {
      t = tmp_buf;
    }
    if ( sentence == 1 ) {
      t = ssym + " " + t + " " + esym; 
    }
    Tokenize( t, targets, ' ' );
    std::vector<std::string>::iterator ti = targets.begin();
    
    double total_logprob = 0.0; // Sum of log2(p)

    // results is an array with the line windowed.
    // The targets are in another array.
    //
    // WHERE DO WE HAPAX? After, or before we window?
    //
    window( classify_line, t, ws, 0, false, results );
    for ( ri = results.begin(); ri != results.end(); ri++ ) {
      
      std::string cl = *ri;
	  
      //l.log( cl ); // debug output
      
      // Classify this buf.
      //
      // Update: If the last word in the context (the word before the
      // target is unknown, we skip Timbl, and take the a priori
      // word probability from the lexicon. This avoids Timbl
      // failing on the top-node and returning a distribution
      // containing everything.
      //
      std::string target = *ti; // was *ri
      
      // Determine word before target. Look it up.
      //
      std::vector<std::string> lcs; // left context, last word.
      Tokenize( cl, lcs, ' ' );
      std::string lc = lcs.at(ws-1);
      std::map<std::string,int>::iterator wfi = wfreqs.find(lc);
      bool lc_unknown = false;
      if ( ( lc != "_") && (wfi == wfreqs.end()) ) {
	lc_unknown = true;
      }
      
      if ( lc_unknown == false ) {
	//My_Experiment->Classify( cl, result, distrib, distance );
	tv = My_Experiment->Classify( cl, vd, distance );
	result = tv->Name();
      } else {
	//
	// Set some dummy values so we can fall through the rest
	// of the code.
	//
	//l.log( "Skipping Timbl for unknown context word:" + lc );
	result   = "__SKIPPED__"; // make sure this is not in lexicon...
	distrib  = "{}";
	distance = 10.0;
      }

      // Grok the distribution returned by Timbl.
      //
      std::map<std::string, double> res;
      //parse_distribution( distrib, res );
      parse_distribution2( vd, res );
      
      // Start calculating.
      //
      double prob = 0.0;
      if ( target == result ) {
	// we got it, no confusion.
	// Count right answers?
      }
      std::map<std::string,double>::iterator foo = res.find(target);
      if ( foo == res.end() ) { // not found. Problem for log.
	//
	// Fall back 1: look up in dictionary.
	//
	std::map<std::string,int>::iterator wfi = wfreqs.find(target);
	if ( wfi == wfreqs.end() ) { // not found.
	  // verbose output removed
	} else {
	  // Found, take p(word in lexicon)
	  //
	  prob = (int)(*wfi).second / (double)total_count ;
	  // verbose output removed
	}
      } else {
	//
	// Our target was in the distribution returned by Timbl.
	// Take the probability of word in distribution.
	//
	prob = (*foo).second;
	// verbose output removed
      }
      //419
   
      // Good Turing smoothing oid. implementeren :)
      //
      if ( prob == 0.0 ) {
	prob = unk_prob; // KLUDGE!!! for testing.
      }
      total_logprob += log2( prob );
      
      if ( verbose == 1 ) {
	l.log( "[" + cl + "] = " + result );
      }
      
      // verbose output removed

      ti++;
    } // end sentence
    // 459

    total_logprob = - total_logprob;
    double avg_logprob = total_logprob / (double)results.size();
    double total_prplx = pow( 2, avg_logprob );
    l.log( "total_prplx = " + to_str(total_prplx) );

    file_out << total_prplx << std::endl;

    std::string p = to_str(total_prplx) + "\n";
    l.log( p );

    // 510

  } // while classify line.

  file_out.close();
  file_in.close();

  // marker file
  //
  std::ofstream filem_out( fr.c_str(), std::ios::out );
  if ( ! filem_out ) {
    l.log( "ERROR: cannot write file." );
    return -1;
  }
  filem_out << the_date_time() << std::endl;
  filem_out.close();

  return 0;
}
#endif


#ifdef TIMBL
/*
  This one is for moses
*/
int server3(Logfile& l, Config& c) {
  l.log( "server3" );
  
  const std::string& timbl      = c.get_value( "timbl" );
  const std::string& ibasefile  = c.get_value( "ibasefile" );
  const int port                = stoi( c.get_value( "port", "1984" ));

  l.inc_prefix();
  l.log( "ibasefile: "+ibasefile );
  l.log( "port:      "+to_str(port) );
  l.log( "timbl:     "+timbl );
  l.dec_prefix();

  std::string distrib;
  std::vector<std::string> distribution;
  std::string result;
  double distance;
  double total_prplx = 0.0;
  const Timbl::ValueDistribution *vd;
  const Timbl::TargetValue *tv;
  
  try {
    Timbl::TimblAPI *My_Experiment = new Timbl::TimblAPI( timbl );
    (void)My_Experiment->GetInstanceBase( ibasefile );

    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct sockaddr_in my_addr;    // my address information
    struct sockaddr_in their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    int numbytes;  
    char buf[MAXDATASIZE];

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      perror("socket");
      exit(1);
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      perror("setsockopt");
      exit(1);
    }

    int ka = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &ka, sizeof(int)) == -1 ) {
      perror("setsockopt");
      exit(1);
    }
      
    my_addr.sin_family = AF_INET;         // host byte order
    my_addr.sin_port = htons(port);       // short, network byte order
    my_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
    memset(my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);
    
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof my_addr) == -1) {
      perror("bind");
      exit(1);
    }
    
    if (listen(sockfd, BACKLOG) == -1) {
      perror("listen");
      exit(1);
    }
    
    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if ( sigaction( SIGCHLD, &sa, NULL ) == -1 ) {
      perror( "sigaction" );
      exit( 1 );
    }

    // loop
    //
    while ( c.get_status() != 0 ) {  // main accept() loop
      sin_size = sizeof their_addr;
      
      if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr,
			   &sin_size)) == -1) {
	perror("accept");
	continue;
      }
      l.log( "Connection from: " +std::string(inet_ntoa(their_addr.sin_addr)));
      
      if ( c.get_status() && ( ! fork() )) { // this is the child process
	close(sockfd); // child doesn't need the listener.	
	
	bool connection_open = true;
	while ( connection_open ) {
	  
	  while ( true ) {
	    
	    if ((numbytes=recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1) {
	      perror("recv");
	      exit(1);
	    }
	    buf[numbytes] = '\0';
	    
	    std::string tmp_buf = buf;//str_clean( buf );
	    tmp_buf = trim( tmp_buf, " \n\r" );
	    
	    if ( tmp_buf == "_CLOSE_" ) {
	      connection_open = false;
	      break;
	    }
	    l.log( "|" + tmp_buf + "|" );
	    
	    std::string classify_line = tmp_buf;
	    
	    if ( classify_line != "" ) {
	      tv = My_Experiment->Classify( classify_line, vd, distance );
	      result = tv->Name();
	      
	      // Grok the distribution returned by Timbl.
	      //
	      std::map<std::string, double> res;
	      parse_distribution2( vd, res ); // was parse_distribution(...)
	      
	      std::string res_str = "0.1";
	      if ( send( new_fd, res_str.c_str(), res_str.length(), 0 ) == -1 ) {
		perror("send");
	      }
	    }
	    
	  } // while true
	  
	} // connection_open
	exit(0);
      } // fork
      close( new_fd );  
    }
  }
  catch ( const std::exception& e ) {
    l.log( "ERROR: exception caught." );
    return -1;
  }
  
  return 0;
}
#else
int server3( Logfile& l, Config& c ) {
  l.log( "No TIMBL support." );
  return -1;
}
#endif 

#ifdef TIMBL
int server3_old(Logfile& l, Config& c) {
  l.log( "server3" );
  
  const std::string& timbl      = c.get_value( "timbl" );
  const std::string& ibasefile  = c.get_value( "ibasefile" );
  const int port                = stoi( c.get_value( "port", "1984" ));

  l.inc_prefix();
  l.log( "ibasefile: "+ibasefile );
  l.log( "port:      "+to_str(port) );
  l.log( "timbl:     "+timbl );
  l.dec_prefix();

  std::string distrib;
  std::vector<std::string> distribution;
  std::string result;
  double distance;
  double total_prplx = 0.0;
  const Timbl::ValueDistribution *vd;
  const Timbl::TargetValue *tv;
  
  try {
    Timbl::TimblAPI *My_Experiment = new Timbl::TimblAPI( timbl );
    (void)My_Experiment->GetInstanceBase( ibasefile );

    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct sockaddr_in my_addr;    // my address information
    struct sockaddr_in their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    int numbytes;  
    char buf[MAXDATASIZE];

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      perror("socket");
      exit(1);
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      perror("setsockopt");
      exit(1);
    }

    int ka = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &ka, sizeof(int)) == -1 ) {
      perror("setsockopt");
      exit(1);
    }
      
    my_addr.sin_family = AF_INET;         // host byte order
    my_addr.sin_port = htons(port);       // short, network byte order
    my_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
    memset(my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);
    
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof my_addr) == -1) {
      perror("bind");
      exit(1);
    }
    
    if (listen(sockfd, BACKLOG) == -1) {
      perror("listen");
      exit(1);
    }
    
    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if ( sigaction( SIGCHLD, &sa, NULL ) == -1 ) {
      perror( "sigaction" );
      exit( 1 );
    }

    // loop
    //
    while ( c.get_status() != 0 ) {  // main accept() loop
      sin_size = sizeof their_addr;

      if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr,
			   &sin_size)) == -1) {
	perror("accept");
	continue;
      }
      l.log( "Connection from: " +std::string(inet_ntoa(their_addr.sin_addr)));

      if ( c.get_status() && ( ! fork() )) { // this is the child process
	close(sockfd); // child doesn't need the listener.	
	
	bool connection_open = true;
	while ( connection_open ) {
	  
	  if ((numbytes=recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1) {
	    perror("recv");
	    exit(1);
	  }
	  buf[numbytes] = '\0';
	  
	  std::string tmp_buf = buf;//str_clean( buf );
	  tmp_buf = trim( tmp_buf, " \n\r" );
	  
	  if ( (tmp_buf == "") || (tmp_buf == "_CLOSE_" ) ) {
	    connection_open = false;
	    break;
	  }

	  // Check for commands? Some kind of protocol?

	  // Let this also window and tokenize input? Seperate
	  // "utility" server which does that?

	  // Read an XML input packet with commands and "stuff" ?

	  // Assume first line is n if we want to send n lines.
	  //
	  int num = stoi( tmp_buf );//.substr(4, std::string::npos) );
	  if ( num == 0 ) {
	    connection_open = false;
	    break;
	  }
	  l.log( "NUM="+to_str(num) );

	  long f1 = l.clock_mu_secs();

	  std::vector<std::string> instances;

	  for ( int i = 0; i < num; i++ ) {

	    if ((numbytes=recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1) {
	      perror("recv");
	      exit(1);
	    }
	    buf[numbytes] = '\0';

	    std::string tmp_buf = buf;//str_clean( buf );
	    tmp_buf = trim( tmp_buf, " \n\r" );

	    instances.push_back( tmp_buf );
	    l.log( to_str(numbytes)+"|" + tmp_buf + "|" );
	  }

	  std::vector<std::string> results_output;

	  for ( int i = 0; i < num; i++ ) {

	    std::string classify_line = instances.at(i);

	    std::vector<std::string> results;
	    std::vector<std::string> targets;
	    std::vector<std::string>::iterator ri;
	    
	    tv = My_Experiment->Classify( classify_line, vd, distance );
	    result = tv->Name();
	    
	    // Grok the distribution returned by Timbl.
	    //
	    std::map<std::string, double> res;
	    parse_distribution2( vd, res ); // was parse_distribution(...)
	    
	    std::string json = result;
	    results_output.push_back( result );
	    
	  } // i

	  for ( int i = 0; i < num; i++ ) {
	    std::string json = results_output.at(i)+"\n";
	    if ( send( new_fd, json.c_str(), json.length(), 0 ) == -1 ) {
	      perror("send");
	    }
	  }

	  long f2 = l.clock_mu_secs();
	  long diff_mu_secs = f2 - f1;
	  
	  l.log( "Processing took (mu-sec): " + to_str(diff_mu_secs) );

	} // connection_open
	exit(0);
      } // fork
      close( new_fd );  
    }
  }
  catch ( const std::exception& e ) {
    l.log( "ERROR: exception caught." );
    return -1;
  }
  
  return 0;
}
#else
int server3_old( Logfile& l, Config& c ) {
  l.log( "No TIMBL support." );
  return -1;
}
#endif

