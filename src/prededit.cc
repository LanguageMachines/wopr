// ---------------------------------------------------------------------------
// $Id: $
// ---------------------------------------------------------------------------

/*****************************************************************************
 * Copyright 2007 - 2011 Peter Berck                                         *
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
#include "Context.h"
#include "prededit.h"

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
  Predictive Editing.

  ./wopr -r pdt -p ibasefile:test/reuters.martin.tok.1e5.l2r0_-a1+D.ibase,timbl:"-a1 +D",filename:test/abc.txt

  ./wopr -r pdt -p ibasefile:./test/reuters.martin.tok.1e5.l2r0_-a1+D.ibase,timbl:"-a1 +D",filename:test/zin01,lc:2

  ./wopr -r pdt -p ibasefile:./test/distr/austen.train.l2r0_-a1+D.ibase,timbl:"-a1 +D",filename:test/zin01,lc:2
*/
#ifdef TIMBL
struct distr_elem {
  std::string name;
  double      freq;
  double      s_freq;
  bool operator<(const distr_elem& rhs) const {
    return freq > rhs.freq;
  }
};
struct salad_elem {
  std::string str;
  // array with the words? frequency? distr_elems?
};
void generate_next( Timbl::TimblAPI*, Config&, std::string, std::vector<distr_elem>& );
void generate_tree( Timbl::TimblAPI*, Config&, Context&, std::vector<std::string>&, int, std::vector<int>&, std::string );

int pdt( Logfile& l, Config& c ) {
  l.log( "predictive editing" );
  const std::string& start           = c.get_value( "start", "" );
  const std::string  filename        = c.get_value( "filename", to_str(getpid()) ); // "test"file
  const std::string& ibasefile       = c.get_value( "ibasefile" );
  const std::string& timbl           = c.get_value( "timbl" );
  int                lc              = stoi( c.get_value( "lc", "2" ));
  int                rc              = stoi( c.get_value( "rc", "0" )); // should be 0
  std::string        ped             = c.get_value( "ds", "" ); // depths
  int                pel             = stoi( c.get_value( "n", "3" )); // length
  bool               show_counts     = stoi( c.get_value( "sc", "0" )) == 1;
  std::string        id              = c.get_value( "id", to_str(getpid()) );

  Timbl::TimblAPI   *My_Experiment;
  std::string        distrib;
  std::vector<std::string> distribution;
  double             distance;

  if ( contains_id(filename, id) == true ) {
    id = "";
  } else {
    id = "_"+id;
  }

  int length = pel; // length of each predicted string
  std::vector<int> depths(length+1, 1);
  if ( ped.size() > length ) {
    ped = ped.substr(0, length );
  }
  for( int i = 0; i < ped.size(); i++) {
    depths.at(length-i) = stoi( ped.substr(i, 1), 32 ); // V=31
  }
  std::string tmp = "n"+to_str(length)+"ds";
  for ( int i = length; i > 0; i-- ) {
    if ( (length-i) < ped.size() ) {
      tmp = tmp + ped[length-i];
    } else {
      tmp = tmp + to_str(depths.at(i)); //this way to get right length/defaults base?
    }
  }

  std::string output_filename = filename + "_" + tmp + id + ".pdt";

  l.inc_prefix();
  l.log( "filename:   "+filename );
  l.log( "ibasefile:  "+ibasefile );
  l.log( "timbl:      "+timbl );
  l.log( "lc:         "+to_str(lc) );
  l.log( "rc:         "+to_str(rc) );
  //l.log( "mode:       "+to_str(mode) );
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

  std::ifstream file_in( filename.c_str() );
  if ( ! file_in ) {
    l.log( "ERROR: cannot load file." );
    return -1;
  }
  std::ofstream file_out( output_filename.c_str(), std::ios::out );
  if ( ! file_out ) {
    l.log( "ERROR: cannot write output file." );
    return -1;
  }

  std::string a_line;
  std::string token;
  std::string result;
  std::vector<std::string> results;
  std::vector<std::string> targets;
  std::vector<std::string>::iterator ri;
  const Timbl::ValueDistribution *vd;
  const Timbl::TargetValue *tv;
  std::vector<std::string> words;
  std::vector<std::string>::iterator wi;
  std::vector<std::string> predictions;
  std::vector<std::string>::iterator pi;
  int correct = 0;
  int wrong   = 0;
  int correct_unknown = 0;
  int correct_distr = 0;
  Timbl::ValueDistribution::dist_iterator it;
  int cnt;
  int distr_count;

  MTRand mtrand;

  std::string ectx; // empty context
  for ( int i = 0; i < lc+rc; i++ ) {
    ectx = ectx + "_ ";
  }

  int ctx_size = lc+rc;
  Context ctx(ctx_size);
  std::vector<distr_elem> res;

  //std::vector<std::string>::iterator it_endm1 = ctx.begin();
  //advance(it_end,4);

  file_out << "# l" << length;
  for ( int i = length; i > 0; i-- ) {
    file_out << " " << depths.at(i); // choices per 'column'
  }
  file_out << " " << ibasefile;
  file_out << std::endl;

  size_t sentence_count = 0;
  size_t prediction_count = 0;
  size_t instance_count = 0;

  int skip = 0;
  double keypresses = 0;
  double keyssaved = 0;

  while( std::getline( file_in, a_line ) ) { 
    if ( a_line == "" ) {
      continue;
    }
    
    words.clear();
    Tokenize( a_line, words );

    // Print the sentence, with counts, and length (keypresses):
    // S0000 we sat and waited for the woman
    //
    file_out << "S" << std::setfill('0') << std::setw(4) << sentence_count << " "; 
    file_out << a_line << " " << a_line.size() << std::endl;

    keypresses += a_line.size();

    instance_count = 0;

    // each word in sentence
    //
    double sentencesaved = 0; // key presses saved in this sentence
    for ( int i = 0; i < words.size(); i++ ) {

      token = words.at(i);

      // We got word 'token' (pretend we just pressed space).
      // Add to context, and call wopr.
      //
      ctx.push( token );

      if ( skip > 0 ) {
	//l.log( "Skipping: " + to_str(skip) );
	// increment instance count? Log? E, edited, excluded
	file_out << "E" << std::setfill('0') << std::setw(4) << sentence_count << "." 
		 << std::setfill('0') << std::setw(4) << instance_count << " "; 
	file_out << ctx << std::endl;
	++instance_count;
	--skip;
	continue;
      }

      // TEST
      //
      // NADEEL: this cannot use right context...only advantage is tribl2...?
      //         unless we shift instance bases after having generated the first 
      //         right context. Tribl2 no use either, we generate self, never
      //         any unknown words in context.
      //
      // IDEA: generated sentences to compare to sentence to be LM'd, if
      // it can be generated, it gets extra score, or score based on 
      // choices every word. That was already an idea.
      //
      std::vector<std::string> strs; // should be a struct with more stuff
      std::string t;
      generate_tree( My_Experiment, c, ctx, strs, length, depths, t );

      // Print the instance, with counts.
      // I0000.0004 waited for
      //
      file_out << "I" << std::setfill('0') << std::setw(4) << sentence_count << "." 
	       << std::setfill('0') << std::setw(4) << instance_count << " "; 
      file_out << ctx << std::endl;

      std::vector<std::string>::iterator si = strs.begin();
      prediction_count = 0;
      double savedhere = 0; // key presses saved in this prediction.
      while ( si != strs.end() ) {

	// We should check if the prediction matches the original sentence.
	// We compare word by word in the sentence and the predicted sequence.
	// keep matching until we mismatch, we don't match "over" mismatched
	// words.
	// A matched word saves it length in number of keypresses. The space after
	// the word is the "select" action, it is not saved.
	//
	// We could "simulate", and hop n words if we predict n words. advance(..)
	// set a skip variable. If not 0, we decrement, and hop over a bit after
	//  "ctx.push( token );" above. Skip the largest (could be more?).
	//
	predictions.clear();
	Tokenize( (*si), predictions );
	pi = predictions.begin(); //start at first word of predicted sequence
	wi = words.begin(); // start at current word in sentence
	advance( wi, i+1 ); //advance to next word in sentence (PJB optimise)
	bool mismatched = false;
	std::string matched = "";
	int words_matched = 0;
	while ( (pi != predictions.end()) && (wi != words.end()) && ( ! mismatched) ) {
	  //l.log( (*pi) + "--" + (*wi) );
	  if ( (*pi) == (*wi) ) {
	    //l.log( "MATCH: " + to_str(prediction_count) + " " + (*pi)  );
	    matched = matched + (*pi) + " ";
	    ++words_matched;
	  } else {
	    mismatched = true;
	  }
	  pi++;
	  wi++;
	}

	// We take largest number of presses, not words. Same result.
	// 
	if ( matched.size() > savedhere+1 ) {
	  //if ( words_matched > skip ) {
	  skip = words_matched;
	  savedhere = matched.size()-1;
	  sentencesaved = savedhere;
	}

	// P0000.0004.0004 his sake and
	//
	// (print only when a match, optional?)
	//
	file_out << "P" << std::setfill('0') << std::setw(4) << sentence_count << "." 
		 << std::setfill('0') << std::setw(4) << instance_count << "." 
		 << std::setfill('0') << std::setw(4) << prediction_count << (*si) << std::endl;

	if ( matched != "" ) {
	  file_out << "M" << std::setfill('0') << std::setw(4) << sentence_count << "." 
		   << std::setfill('0') << std::setw(4) << instance_count << "." 
		   << std::setfill('0') << std::setw(4) << prediction_count 
		   << " " << matched << matched.size()-1 << std::endl; // -1 for trailing space
	}
	
	prediction_count++;
	si++;
      }
      strs.clear();
      
      keyssaved += savedhere;
      
      ++instance_count;
    } // i over words

    // Output Result for this sentence.
    //
    file_out << "R" << std::setfill('0') << std::setw(4) << sentence_count << " "; 
    file_out << a_line << " " << sentencesaved << std::endl;

    ++sentence_count;
  }

  // Output Totals
  //
  file_out << "T " << keypresses << " " << keyssaved << " " << ((float)keyssaved/keypresses)*100.0 << std::endl;

  l.log( "Keypresses: " + to_str(keypresses) );
  l.log( "Saved     : " + to_str(keyssaved) );

  c.add_kv( "pdt_file", output_filename );
  l.log( "SET gpdt_file to "+output_filename );

  return 0;
}  

// Should implement caching? Threading, ...
// Cache must be 'global', contained in c maybe, or a parameter.
//
void generate_next( Timbl::TimblAPI* My_Experiment, Config& c, std::string instance, std::vector<distr_elem>& distr_vec ) {

  std::string distrib;
  std::vector<std::string> distribution;
  double distance;
  double total_prplx = 0.0;
  const Timbl::ValueDistribution *vd;
  const Timbl::TargetValue *tv;
  std::string result;

  tv = My_Experiment->Classify( instance, vd, distance );
  if ( ! tv ) {
    //l.log( "ERROR: Timbl returned a classification error, aborting." );
    //error
  }
  
  result = tv->Name();	
  size_t res_freq = tv->ValFreq();
  
  double res_p = -1;
  bool target_in_dist = false;
  int target_freq = 0;
  int cnt = vd->size();
  int distr_count = vd->totalSize();
  
  // Grok the distribution returned by Timbl.
  //
  Timbl::ValueDistribution::dist_iterator it = vd->begin();
  while ( it != vd->end() ) {
    
    std::string tvs  = it->second->Value()->Name();
    double      wght = it->second->Weight(); // absolute frequency.

    distr_elem  d;
    d.name   = tvs;
    d.freq   = wght;
    d.s_freq = wght;
    distr_vec.push_back( d );

    ++it;
  } // end loop distribution

  // We can take the top-n here, and recurse into generate_next (add a depth parameter)
  // with each top-n new instance. OR write a wrapper fun.
  
}

// Recursive?
//
void generate_tree( Timbl::TimblAPI* My_Experiment, Config& c, Context& ctx, std::vector<std::string>& strs, int length, std::vector<int>& depths, std::string t ) {

  if ( length == 0 ) {
    // here we should save/print the whole "string" (?)
    //std::cout << "STRING: " << t << std::endl;
    strs.push_back( t );
    return;
  }

  const Timbl::ValueDistribution *vd;
  const Timbl::TargetValue *tv;
  std::string result;
  Timbl::ValueDistribution::dist_iterator it = vd->begin();

  // generate top-n, for each, recurse one down.
  // Need a different res in generate_next, and keep another (kind of) res
  // in this generate_tree.
  //
  std::string instance = ctx.toString() + " ?";
  std::vector<distr_elem> res;
  generate_next( My_Experiment, c, instance, res );

  // std::cerr << "RESSIZE=" << res.size() << std::endl;
  // Should have a limit on res.size(), default distr. is crap, no use
  // to continue with that. default happens not often, self generated instances.
  //
  /*  if ( res.size() > 10000 ) {
    return;
    }*/

  sort( res.begin(), res.end() );
  std::vector<distr_elem>::iterator fi = res.begin();
  int cnt = depths.at( length );
  while ( (fi != res.end()) && (--cnt >= 0) ) { 
    
    //for ( int i = 5-length; i > 0; i--) { std::cout << "  ";}
    //std::cerr << length << ":" << cnt  << " " << ctx << " -> " << (*fi).name /* << ' ' << (*fi).freq */ << std::endl;

    // Make new context, recurse
    //
    Context new_ctx = Context(ctx);
    new_ctx.push(  (*fi).name );

    generate_tree( My_Experiment, c, new_ctx, strs, length-1, depths, t+" "+(*fi).name );//+"/"+to_str(res.size()) );
    
    fi++;
  }
  
}
#else
int pdt( Logfile& l, Config& c ) {
  l.log( "No TIMBL support." );
  return -1;
}  
#endif

