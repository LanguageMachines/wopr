// $Id: woprsm.cc 3329 2009-12-10 07:49:45Z pberck $

// ---------------------------------------------------------------------------
//  Includes.
// ---------------------------------------------------------------------------

#if HAVE_CONFIG_H
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/msg.h>

#include <stdlib.h>  
#include <string.h>  
#include <getopt.h>

#include "qlog.h"
#include "util.h"

#ifdef TIMBLSERVER
#include "SocketBasics.h"
#endif

int run_file( Logfile&, const std::string&,
	      const std::string&, const std::string& );

// ---------------------------------------------------------------------------
//  Code.
// ---------------------------------------------------------------------------

/*!
  \brief Main.
*/
int main( int argc, char* argv[] ) {

  Logfile l;
  std::string blurb = "Starting woprst " + std::string(VERSION);
  l.log( blurb );
  l.log( "PID: "+to_str(getpid(),6,' ')+" PPID: "+to_str(getppid(),6,' ') );

  int         verbose  = 0;
  std::string filename = "";
  std::string port     = "1984";
  std::string host     = "localhost";

  while ( 1 ) {
    int option_index = 0;
    int c;
    static struct option long_options[] = {
      //{"name, has_arg, *flag, val"} 
      //no_argument, required_argument, optional_argument
      {"host", required_argument, 0, 0},
      {"port", required_argument, 0, 0},
      {"file", optional_argument, 0, 0},
      {"verbose", no_argument, 0, 0},
      {0, 0, 0, 0}
    };

    c = getopt_long(argc, argv, "h:p:f:v", long_options, &option_index);
    if (c == -1) {
      break;
    }
	     
    switch ( c ) {
      
    case 0:
      if ( long_options[option_index].name == "verbose" ) {
	verbose = 1;
      }
      if ( long_options[option_index].name == "port" ) {
	port = optarg;
      }
      if ( long_options[option_index].name == "host" ) {
	host = optarg;
      }
      if ( long_options[option_index].name == "file" ) {
	filename = optarg;
      }
      break;
      
    case 'v':
      verbose = 1;
      break;
      
    case 'p':
      port = optarg;
      break;

    case 'h':
      host = optarg;
      break;

    case 'f':
      filename = optarg;
      break;

    default:
      //printf ("?? getopt returned character code 0%o ??\n", c);
      l.log( "ERROR: invalid option." );
      exit( 1 );
    }
  } // while

  run_file( l, filename, host, port );

  return 0;
}

// ---------------------------------------------------------------------------

/*
durian:test_pplxs pberck$ ../wopr -r server_sc_nf -p ibasefile:OpenSub-english.train.txt.l2r0.hpx1_-a1+D.ibase,timbl:"-a1 +D",lexicon:OpenSub-english.train.txt.lex,verbose:1,mld:1,mwl:2,max_ent:100,max_distr:100,keep:1

and

durian:wopr pberck$ ./woprst -f /Users/pberck/prog/trunk/sources/wopr/test_pplxs/austen.l100.l2r0
11:27:26.12: Starting woprst 1.28.2
11:27:26.15: PID:  50381 PPID:  38908
11:27:26.15: Running: /Users/pberck/prog/trunk/sources/wopr/test_pplxs/austen.l100.l2r0
11:27:26.15: true
11:27:26.15: Starting test...
1305710846156838
1305710851907106
11:27:31.90: End test...
11:27:31.90: microseconds: 5750268
11:27:31.90: microseconds/915: 6284

-- Bench 1 --

durian:test_pplxs pberck$ ../wopr -r server_sc_nf -p ibasefile:OpenSub-english.train.txt.l2r0.hpx1_-a1+D.ibase,timbl:"-a1 +D",lexicon:OpenSub-english.train.txt.lex,verbose:1,mld:1,mwl:2,max_ent:100,max_distr:VAR,keep:1

Results from second run:
./woprst -f /Users/pberck/prog/trunk/sources/wopr/test_pplxs/austen.l100.l2r0

max_distr: 1
11:32:44.12: microseconds: 5701331
11:32:44.12: microseconds/915: 6230

max_distr: 100
11:33:10.87: microseconds: 5799525
11:33:10.87: microseconds/915: 6338

max_distr: 10000
11:33:51.06: microseconds: 6418074
11:33:51.06: microseconds/915: 7014

max_distr: 100000
:36:40.16: microseconds: 11516600
11:36:40.16: microseconds/915: 12586

max_distr: 1000000
11:34:41.69: microseconds: 10622395
11:34:41.69: microseconds/915: 11609

max_distr: 100000000
11:35:23.03: microseconds: 11924097
11:35:23.03: microseconds/915: 13031

-- Bench 2, forking server

max_distr: 1
11:38:13.26: microseconds: 5814619
11:38:13.26: microseconds/915: 6354

max_distr: 100
11:39:04.59: microseconds: 6144092
11:39:04.59: microseconds/915: 6714

max_distr: 10000
11:39:38.71: microseconds: 6615839
11:39:38.71: microseconds/915: 7230

max_distr: 100000
11:40:35.84: microseconds: 10764003
11:40:35.84: microseconds/915: 11763

max_distr: 1000000
11:43:54.23: microseconds: 11215801
11:43:54.23: microseconds/915: 12257

max_distr: 100000000
11:43:15.93: microseconds: 10867859
11:43:15.93: microseconds/915: 11877
*/
int run_file( Logfile& l, const std::string& filename,
	      const std::string& host, const std::string& port) {

  l.log( "Running: "+filename );

  Sockets::ClientSocket cs;
  bool res = cs.connect( host.c_str(), port.c_str() );

  std::ifstream infile( filename.c_str() );
  if ( ! infile ) {
    l.log( "ERROR: cannot load file." );
    return -1;
  }

  l.log( "Starting test..." );

  std::string a_line;
  long u_secs0;
  long u_secs1;
  int lines = 0;

  u_secs0 = clock_u_secs();
  std::cout << u_secs0 << std::endl;
  while( std::getline( infile, a_line ) ) {    
    //l.log( a_line );
    (void)cs.write( a_line +"\n" );
    std::string answer;
    cs.read(answer);
    //l.log( answer );
    ++lines;
  }
  u_secs1 = clock_u_secs();
  std::cout << u_secs1 << std::endl;

  l.log( "End test..." );
  l.log( "microseconds: "+to_str(u_secs1-u_secs0) );
  l.log( "microseconds/"+to_str(lines)+": "+to_str((u_secs1-u_secs0)/lines) );

  infile.close();
}
