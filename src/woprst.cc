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

#include "cache.h"

#ifdef TIMBLSERVER
#include "SocketBasics.h"
#endif

int run_file( Logfile&, const std::string&,
	      const std::string&, const std::string& );
int run_cache( Logfile&, const std::string& );
int run_ping( Logfile& , const std::string& , const std::string& );

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

  l.log("new Cache( 10 )");
  Cache *cache = new Cache( 10 );

  l.log("add( '1 b c', 'one' )");
  cache->add( "1 b c", "one" );
  cache->print();

  l.log("add( '1 b c', 'one' )");
  cache->add( "1 b c", "one" );
  cache->print();

  l.log("add( '2 b d', 'two' )");
  cache->add( "2 b d", "two" );
  cache->print();

  l.log("add( '3 b c', 'three' )");
  cache->add( "3 b c", "three" );
  cache->print();

  l.log("add( '1 b c', 'one' )");
  cache->add( "1 b c", "one" );
  cache->print();

  l.log("add( '4 b c', 'four' )");
  cache->add( "4 b c", "four" );
  cache->print();
  l.log( cache->stat() );

  std::cout << "cache get:" << cache->get( "1 b c" ) << std::endl;
  std::cout << "cache get:" << cache->get( "2 b d" ) << std::endl;
  std::cout << "cache get:" << cache->get( "3 b c" ) << std::endl;
  std::cout << "cache get:" << cache->get( "4 b c" ) << std::endl;

  l.log("new Cache( 2 )");
  cache = new Cache( 2 );
  l.log( cache->stat() );

  l.log("add( '1 b c', 'one' )");
  cache->add( "1 b c", "one" );
  cache->print();
  l.log( cache->stat() );

  l.log("add( '1 b c', 'one' )");
  cache->add( "1 b c", "one" );
  cache->print();
  l.log( cache->stat() );

  l.log("add( '2 b d', 'two' )");
  cache->add( "2 b d", "two" );
  cache->print();
  l.log( cache->stat() );

  l.log("add( '3 b c', 'three' )");
  cache->add( "3 b c", "three" );
  cache->print();
  l.log( cache->stat() );

  l.log("add( '1 b c', 'one' )");
  cache->add( "1 b c", "one" );
  cache->print();
  l.log( cache->stat() );

  l.log("add( '4 b c', 'four' )");
  cache->add( "4 b c", "four" );
  cache->print();
  l.log( cache->stat() );

  std::cout << "cache get:" << cache->get( "1 b c" ) << std::endl;
  std::cout << "cache get:" << cache->get( "2 b d" ) << std::endl;
  std::cout << "cache get:" << cache->get( "3 b c" ) << std::endl;
  std::cout << "cache get:" << cache->get( "4 b c" ) << std::endl;


  if ( filename != "" ) {
    run_file( l, filename, host, port );
  } else {
    run_ping( l, host, port );
  }

  //run_cache( l, filename );
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

Other experiment:

in /Users/pberck/prog/trunk/sources/wopr/test/esslli2010/correct

~/prog/trunk/sources/wopr/wopr -r server_sc -p ibasefile:austen.train.l2r0_-a1+D.ibase,timbl:"-a1 +D",lexicon:austen.train.lex,verbose:1,mld:1,mwl:2,max_ent:100,max_distr:1000,keep:1

durian:wopr pberck$ ./woprst -f  /Users/pberck/prog/trunk/sources/wopr/test/esslli2010/correct/austen.test.l2r0

10:16:11.02: microseconds: 8966595
10:16:11.02: microseconds/8308: 1079

same, but server_sc_nf (with cache):
10:16:44.72: microseconds: 7233901
10:16:44.72: microseconds/8308: 870

second run with cache:
10:17:03.89: microseconds: 1542052
10:17:03.89: microseconds/8308: 185

Chaos:
                                         server_sc
pberck@chaos:/exp/pberck/wopr$ ./wopr -r server_sc_nf -p ibasefile:/exp/antalb/valkuil-servers/DUTCH-TWENTE-ILK.tok.1e5.l3r3.ibase,timbl:"-a1 +D",lexicon:/exp/antalb/sources/valkuil/valkuil-servers/DUTCH-TWENTE-ILK.tok.1e5.lex,port:1984,keep:1,mwl:3,cs:5000

pberck@chaos:/exp/pberck/wopr$ head -n10000  /exp/pberck/wopr_exps1/timbl-a1+D/ibase_1e6.l3r3/DUTCH-TWENTE-ILK.tok.l10000.l3r3 >tst.l3r3.10000
pberck@chaos:/exp/pberck/wopr$ ./woprst -f tst.l3r3.10000 

server_sc
cache: 1:
10:59:35.37: microseconds: 43331205
10:59:35.37: microseconds/10000: 4333

cache: 10000:
10:57:58.18: microseconds: 46286894
10:57:58.18: microseconds/10000: 4628

cache: 100000:
11:01:21.67: microseconds: 42992410
11:01:21.67: microseconds/10000: 4299
(11:01:21.67: 9485:shmer:9485:515:9485:0:0)

server_sc_nf
cache: 1:
11:03:06.20: microseconds: 41519889
11:03:06.20: microseconds/10000: 4151
(11:03:06.20: 1:shmer:10000:0:10000:9999:0)

Run 2:
11:06:10.88: microseconds: 40837104
11:06:10.88: microseconds/10000: 4083
(11:06:10.88: 1:shmer:20000:0:20000:19999:0)

cache: 5000:
11:10:54.40: microseconds: 38985134
11:10:54.40: microseconds/10000: 3898
(11:10:54.40: 5000:shmer:9494:506:9494:4494:0)

Run 2:
11:11:54.57: microseconds: 38608015
11:11:54.57: microseconds/10000: 3860
(11:11:54.57: 5000:shmer:18985:1015:18985:13985:0)

cache: 10000:
11:08:06.04: microseconds: 40839831
11:08:06.04: microseconds/10000: 4083
(11:08:06.04: 9485:shmer:9485:515:9485:0:0)

Run 2:
11:08:46.21: microseconds: 849391
11:08:46.21: microseconds/10000: 84
(11:08:46.21: 9485:shmer:9485:10515:9485:0:0)

cache: 1000000:
11:15:08.90: microseconds: 39774270
11:15:08.90: microseconds/10000: 3977
(11:15:08.90: 9485:shmer:9485:515:9485:0:0)

Run 2:
11:16:15.99: microseconds: 726631
11:16:15.99: microseconds/10000: 72
(11:16:15.99: 9485:shmer:9485:10515:9485:0:0)
*/
int run_file( Logfile& l, const std::string& filename,
	      const std::string& host, const std::string& port) {

  l.log( "Running: "+filename );

  Sockets::ClientSocket cs;
  try {
    bool res = cs.connect( host.c_str(), port.c_str() );
  } catch ( const std::exception& e ) {
    l.log( "ERROR: exception caught." );
    return -1;
  }

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
    if ( answer != "__EMPTY__" ) {
      l.log( a_line + "/" + answer );
    }
    ++lines;
    delay(100);
  }
  u_secs1 = clock_u_secs();
  std::cout << u_secs1 << std::endl;
  l.log( "End test..." );
  l.log( "microseconds: "+to_str(u_secs1-u_secs0) );
  l.log( "microseconds/"+to_str(lines)+": "+to_str((u_secs1-u_secs0)/lines) );

  infile.close();
}

int run_cache( Logfile& l, const std::string& filename ) {

  l.log( "Running: "+filename );

  std::ifstream infile( filename.c_str() );
  if ( ! infile ) {
    l.log( "ERROR: cannot load file." );
    return -1;
  }

  l.log( "Starting test..." );

  Cache *cache = new Cache(100000);

  std::string a_line;
  long u_secs0;
  long u_secs1;
  int lines = 0;

  u_secs0 = clock_u_secs();
  std::cout << u_secs0 << std::endl;
  while( std::getline( infile, a_line ) ) {    
    //l.log( a_line );
    cache->add( a_line, a_line );
    ++lines;
  }
  u_secs1 = clock_u_secs();
  std::cout << u_secs1 << std::endl;
  l.log( cache->stat() );
  l.log( "End test..." );
  l.log( "microseconds: "+to_str(u_secs1-u_secs0) );
  l.log( "microseconds/"+to_str(lines)+": "+to_str((u_secs1-u_secs0)/lines) );

  infile.close();

  std::ifstream infile2( filename.c_str() );
  if ( ! infile2 ) {
    l.log( "ERROR: cannot load file." );
    return -1;
  }

  l.log( "Starting test 2..." );
  lines = 0;

  u_secs0 = clock_u_secs();
  std::cout << u_secs0 << std::endl;
  while( std::getline( infile2, a_line ) ) {    
    //l.log( a_line );
    cache->get( a_line );
    ++lines;
  }
  u_secs1 = clock_u_secs();
  std::cout << u_secs1 << std::endl;
  std::cout << cache->stat() << std::endl;
  l.log( "End test..." );
  l.log( "microseconds: "+to_str(u_secs1-u_secs0) );
  l.log( "microseconds/"+to_str(lines)+": "+to_str((u_secs1-u_secs0)/lines) );

  infile2.close();
}

// Preted te be a load balancer
//
int run_ping( Logfile& l, const std::string& host, const std::string& port) {

  l.log( "Running ping. " );

  long u_secs0;
  long u_secs1;
  int count = 10000;

  u_secs0 = clock_u_secs();
  std::cout << u_secs0 << std::endl;
  while( --count != 0 ) {    

    l.log( "ping" );
    Sockets::ClientSocket cs;
    try {
      bool res = cs.connect( host.c_str(), port.c_str() );
      l.log( "res="+to_str(res) );
      delay(1000);
    } catch ( const std::exception& e ) {
      l.log( "ERROR: exception caught." );
      return -1;
    }
    cs.close();
    delay(10000000); // 10 secs
  }
  u_secs1 = clock_u_secs();
  std::cout << u_secs1 << std::endl;
  l.log( "End test..." );
  l.log( "microseconds: "+to_str(u_secs1-u_secs0) );
}
