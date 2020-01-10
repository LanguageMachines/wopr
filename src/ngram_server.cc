/*****************************************************************************
 * Copyright 2007 - 2020 Peter Berck, Ko vd Sloot                            *
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
#include <cerrno>

#include <math.h>

#include "wopr/qlog.h"
#include "wopr/Config.h"
#include "wopr/util.h"
#include "wopr/runrunrun.h"
#include "wopr/ngram_server.h"
#include "wopr/ngram_elements.h"
#include "wopr/ngrams.h"

#ifdef TIMBLSERVER
#include "ticcutils/SocketBasics.h"
#endif

// ---------------------------------------------------------------------------
//  Code.
// ---------------------------------------------------------------------------

#ifdef TIMBLSERVER
/*
  On margretetorp:
  terminal1: pberck@margretetorp ~/work/prog/sources/wopr $
             ./wopr -r ngs -p port:1984,ngl:test/austen.txt.ngl3f0,resm:1,verbose:1
  terminal2: echo "Admiral hates trouble"  | nc localhost 1984
*/
int ngram_server(Logfile& l, Config& c) {
  l.log( "ngs" );

  const std::string& ngl_filename    = c.get_value( "ngl" );
  const std::string& counts_filename = c.get_value( "counts" );
  int                n               = my_stoi( c.get_value( "n", "3" ));
  //  std::string        id              = c.get_value( "id", "" );
  const std::string  port            = c.get_value( "port", "1984" );
  //const int mode                     = my_stoi( c.get_value( "mode", "0" ));
  const int resm                     = my_stoi( c.get_value( "resm", "0" ));
  const int verbose                  = my_stoi( c.get_value( "verbose", "0" ));
  const int keep                     = my_stoi( c.get_value( "keep", "0" ));
  const int sos                      = my_stoi( c.get_value( "sos", "0" ));
  const int eos                      = my_stoi( c.get_value( "eos", "0" ));

  l.inc_prefix();

  l.log( "port:      "+port );
  //l.log( "mode:      "+to_str(mode) );
  l.log( "resm:      "+to_str(resm) );
  l.log( "keep:      "+to_str(keep) );
  l.log( "ngl file:  "+ngl_filename );
  l.log( "counts:    "+counts_filename );
  l.log( "n:         "+to_str(n) );
  l.log( "verbose:   "+to_str(verbose) );
  l.log( "sos:       "+to_str(sos) );
  l.log( "eos:       "+to_str(eos) );
  l.dec_prefix();

  std::ifstream file_ngl( ngl_filename.c_str() );
  if ( ! file_ngl ) {
    l.log( "ERROR: cannot load file." );
    return -1;
  }

  std::string a_line;
  std::map<std::string,double> ngrams; // NB no ngl_elem here!

  l.log( "Reading ngrams..." );

  // format: ngram freq prob
  // ngram can contain spaces. freq is ignored at the mo.
  // NB: input and my_stod are not checked for errors (TODO).
  //
  while( std::getline( file_ngl, a_line ) ) {
    size_t pos  = a_line.rfind(' ');
    std::string prob_str = a_line.substr(pos+1);
    double prob = my_stod( prob_str );
    size_t pos1 = a_line.rfind(' ', pos-1);
    std::string ngram = a_line.substr(0, pos1);
    ngrams[ngram] = prob;
  }
  file_ngl.close();

  Sockets::ServerSocket server;

  if ( ! server.connect( port )) {
    l.log( "ERROR: cannot start server: "+server.getMessage() );
    return 1;
  }
  if ( ! server.listen() ) {
    l.log( "ERROR: cannot listen. ");
    return 1;
  };

  l.log( "Starting server..." );
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

    while ( newSock->read( buf ) ) {
      buf = trim( buf, " \n\r" );
      if ( sos > 0 ) {
	buf = "<s> " + buf;
      }
      if ( eos > 0 ) {
	buf = buf + " </s>";
      }
      if ( verbose > 0 ) {
	l.log( "[" + buf + "]" );
      }
      // process one line...

      std::vector<ngram_elem> best_ngrams;
      std::vector<std::string> results;
      (void)ngram_one_line( buf, n, ngrams, best_ngrams, results, l );

      double sum_l10probs = 0.0;
      int wc = 0;
      std::vector<ngram_elem>::iterator ni;
      for( ni = best_ngrams.begin()+sos; ni != best_ngrams.end(); ++ni ) {
	double p = (*ni).p;
	++wc;
	std::string ngram = (*ni).ngram;
	if ( verbose > 1 ) {
	  if ( ngram == "" ) {
	    ngram = "<unk>";
	  }
	  l.log( ngram+"/"+to_str(p) ) ;
	}
	if ( p > 0 ) {
	  sum_l10probs += log10( p );
	} else {
	  sum_l10probs += -99;
	}
      }
      if ( resm == 0 ) {
	sum_l10probs /= wc;
      }
      if ( verbose > 0 ) {
	l.log( to_str(sum_l10probs));
      }
      newSock->write( to_str(sum_l10probs));

      buf.clear();
    }
    delete newSock;
  }

  return 0;
}
#else
int ngram_server( Logfile& l, Config& c ) {
  l.log( "No TIMBLSERVER support." );
  return -1;
}
#endif

// ---------------------------------------------------------------------------
