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
#include "PDT.h"

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
/*void generate_next( Timbl::TimblAPI*, Config&, std::string, std::vector<distr_elem>& );
void generate_tree( Timbl::TimblAPI*, Config&, Context&, std::vector<std::string>&, int, std::vector<int>&, std::string );
int explode(std::string, std::vector<std::string>&);
void window_word_letters(std::string, std::string, int, Context&, std::vector<std::string>&);
void window_words_letters(std::string, int, Context&, std::vector<std::string>&);*/

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
  bool               matchesonly     = stoi( c.get_value( "mo", "0" )) == 1; // show only matches
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
  l.log( "mo:         "+to_str(matchesonly) );
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
  long keypresses = 0;
  long keyssaved = 0;

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
    long sentenceksaved = 0; // key presses saved in this sentence
    long sentencewsaved = 0; // words saved in this sentence
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
      generate_tree( My_Experiment, ctx, strs, length, depths, t );

      // Print the instance, with counts.
      // I0000.0004 waited for
      //
      file_out << "I" << std::setfill('0') << std::setw(4) << sentence_count << "." 
	       << std::setfill('0') << std::setw(4) << instance_count << " "; 
      file_out << ctx << std::endl;

      std::vector<std::string>::iterator si = strs.begin();
      prediction_count = 0;
      long savedhere = 0; // key presses saved in this prediction.
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
	  sentenceksaved += savedhere;
	  sentencewsaved += words_matched;
	}

	// P0000.0004.0004 his sake and
	//
	// (print only when a match, optional?)
	//
	if ( ( matchesonly && (matched != "") ) || ( matchesonly == false ) ) {
	file_out << "P" << std::setfill('0') << std::setw(4) << sentence_count << "." 
		 << std::setfill('0') << std::setw(4) << instance_count << "." 
		 << std::setfill('0') << std::setw(4) << prediction_count << (*si) << std::endl;

	if ( matched != "" ) {
	  file_out << "M" << std::setfill('0') << std::setw(4) << sentence_count << "." 
		   << std::setfill('0') << std::setw(4) << instance_count << "." 
		   << std::setfill('0') << std::setw(4) << prediction_count 
		   << " " << matched << matched.size()-1 << std::endl; // -1 for trailing space
	}
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
    file_out << /*a_line << " " <<*/ sentencewsaved << " " << sentenceksaved << std::endl;

    ctx.reset();

    ++sentence_count;
  }

  // Output Totals
  //
  file_out << "T " << keypresses << " " << keyssaved << " " << ((double)keyssaved/keypresses)*100.0 << std::endl;

  l.log( "Keypresses: " + to_str(keypresses) );
  l.log( "Saved     : " + to_str(keyssaved) );

  c.add_kv( "pdt_file", output_filename );
  l.log( "SET pdt_file to "+output_filename );

  return 0;
}  

// Two instance bases.
//
int pdt2( Logfile& l, Config& c ) {
  l.log( "predictive editing2" );
  const std::string& start           = c.get_value( "start", "" );
  const std::string  filename        = c.get_value( "filename", to_str(getpid()) ); // "test"file
  const std::string& ibasefile0      = c.get_value( "ibasefile0" );
  const std::string& timbl0          = c.get_value( "timbl0" );
  const std::string& ibasefile1      = c.get_value( "ibasefile1" );
  const std::string& timbl1          = c.get_value( "timbl1" );
  int                lc0             = stoi( c.get_value( "lc0", "2" ));
  int                rc0             = stoi( c.get_value( "rc0", "0" )); // should be 0
  int                lc1             = stoi( c.get_value( "lc1", "2" ));
  int                rc1             = stoi( c.get_value( "rc1", "0" )); // should be 0
  std::string        ped             = c.get_value( "ds", "" ); // depths
  int                pel             = stoi( c.get_value( "n", "3" )); // length
  std::string        dl              = c.get_value( "dl", "3" ); // letter depth 
  bool               matchesonly     = stoi( c.get_value( "mo", "0" )) == 1; // show only matches
  std::string        id              = c.get_value( "id", to_str(getpid()) );
  int                mlm             = stoi( c.get_value( "mlm", "0" )); // minimum letter match

  Timbl::TimblAPI   *My_Experiment0;
  Timbl::TimblAPI   *My_Experiment1;
  std::string        distrib;
  std::vector<std::string> distribution;
  double             distance;

  if ( contains_id(filename, id) == true ) {
    id = "";
  } else {
    id = "_"+id;
  }

  int length0 = 1; // hardcoded
  std::string ped0 = dl;
  std::vector<int> depths0(length0+1, 1);
  for( int i = 0; i < ped0.size(); i++) {
    depths0.at(length0-i) = stoi( ped0.substr(i, 1), 32 ); // V=31
  }
  //
  int length = pel; // length of each predicted string
  std::vector<int> depths(length+1, 1);
  if ( ped.size() > length ) {
    ped = ped.substr(0, length );
  }
  for( int i = 0; i < ped.size(); i++) {
    depths.at(length-i) = stoi( ped.substr(i, 1), 32 ); // V=31
  }

  std::string tmp = "dl"+dl;
  tmp = tmp + "_n"+to_str(length)+"ds";
  for ( int i = length; i > 0; i-- ) {
    if ( (length-i) < ped.size() ) {
      tmp = tmp + ped[length-i];
    } else {
      tmp = tmp + to_str(depths.at(i)); //this way to get right length/defaults base?
    }
  }

  std::string output_filename = filename + "_" + tmp + id + ".pdt2"; //pdt2?

  if ( file_exists(l, c, output_filename) ) {
    l.log( "OUTPUT exists, not overwriting." );
    c.add_kv( "pdt2_file", output_filename );
    l.log( "SET pdt2_file to "+output_filename );
    return 0;
  }

  l.inc_prefix();
  l.log( "filename:   "+filename );
  l.log( "ibasefile:  "+ibasefile0 );
  l.log( "timbl:      "+timbl0 );
  l.log( "lc:         "+to_str(lc0) );
  l.log( "rc:         "+to_str(rc0) );
  l.log( "ibasefile:  "+ibasefile1 );
  l.log( "timbl:      "+timbl1 );
  l.log( "lc:         "+to_str(lc1) );
  l.log( "rc:         "+to_str(rc1) );
  l.log( "mo:         "+to_str(matchesonly) );
  l.log( "OUTPUT:     "+output_filename );
  l.dec_prefix();

  try {
    My_Experiment0 = new Timbl::TimblAPI( timbl0 );
    if ( ! My_Experiment0->Valid() ) {
      l.log( "Timbl Experiment0 is not valid." );
      return 1;      
    }
    (void)My_Experiment0->GetInstanceBase( ibasefile0 );
    if ( ! My_Experiment0->Valid() ) {
      l.log( "Timbl Experiment0 is not valid." );
      return 1;
    }
    // My_Experiment->Classify( cl, result, distrib, distance );
    My_Experiment1 = new Timbl::TimblAPI( timbl1 );
    if ( ! My_Experiment1->Valid() ) {
      l.log( "Timbl Experiment1 is not valid." );
      return 1;      
    }
    (void)My_Experiment1->GetInstanceBase( ibasefile1 );
    if ( ! My_Experiment1->Valid() ) {
      l.log( "Timbl Experiment1 is not valid." );
      return 1;
    }
  } catch (...) {
    l.log( "Cannot create Timbl Experiments..." );
    return 1;
  }
  l.log( "Instance bases loaded." );

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
  std::string letter;
  std::string result;
  std::vector<std::string> results;
  std::vector<std::string> targets;
  std::vector<std::string>::iterator ri;
  const Timbl::ValueDistribution *vd;
  const Timbl::TargetValue *tv;
  std::vector<std::string> words;
  std::vector<std::string> letters;
  std::vector<std::string>::iterator wi;
  std::vector<std::string>::iterator li;
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

  // Letter context, from ibase0
  //
  int ctx_size0 = lc0+rc0;
  Context ctx0(ctx_size0);

  // Word context, from ibase1
  //
  int ctx_size1 = lc1+rc1;
  Context ctx1(ctx_size1);

  std::vector<distr_elem> res;

  file_out << "# l" << length;
  for ( int i = length; i > 0; i-- ) {
    file_out << " " << depths.at(i); // choices per 'column'
  }
  file_out << std::endl;
  file_out << "# " << ibasefile0 << " " << ibasefile1;
  file_out << std::endl;

  size_t sentence_count = 0;
  size_t prediction_count = 0;
  size_t instance_count = 0;

  int skip = 0;
  long keypresses = 0;
  long keyssaved = 0;
  long letterssaved = 0;

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
    long sentenceksaved = 0; // key presses saved in this sentence
    long sentencewsaved = 0; // words saved in this sentence
    long sletterssaved  = 0; // letter keys saved in this sentence

    for ( int i = 0; i < words.size(); i++ ) {

      token = words.at(i);

      // We got word 'token' (pretend we just pressed space).
      // Add to context, and call wopr.
      //
      ctx1.push( token );

      if ( skip > 0 ) {
	//l.log( "Skipping: " + to_str(skip) );
	// increment instance count? Log? E, edited, excluded
	//
	file_out << "E" << std::setfill('0') << std::setw(4) << sentence_count << "." 
		 << std::setfill('0') << std::setw(4) << instance_count << " "; 
	file_out << ctx1 << std::endl;
	++instance_count;
	--skip;

	// We need to skip the letter context as well now.
	//
	(void)explode( token, letters );
	for ( int j = 0; j < letters.size(); j++ ) {
	  letter = letters.at(j);
	  ctx0.push( letter );
	}
	ctx0.push( "_" );

	continue;
      }

      // Moved up from below.
      //
      file_out << "I" << std::setfill('0') << std::setw(4) << sentence_count << "." 
	       << std::setfill('0') << std::setw(4) << instance_count << " "; 
      file_out << ctx1 << std::endl;

      std::vector<std::string> strs; // should be a struct with more stuff
      std::string t;

      // Explode word into letters.
      // Loop over all letters here, do predictions, just to get
      // starting context for the ibase1 classifier, which continues word based.
      // We need a data struct here to save data in this loop.
      //
      letters.clear();
      (void)explode( token, letters );
      int lsaved = 0;
      //l.log( "Inside "+token+", letters="+to_str(letters.size()) );
      for ( int j = 0; j < letters.size(); j++ ) {

	// get the letter we "typed", and add it to the context used
	// to predict the rest of the word.
	//
	letter = letters.at(j);
	ctx0.push( letter );
	//l.log( "ctx0="+ctx0.toString() );

	/*
	file_out << "L" << std::setfill('0') << std::setw(4) << sentence_count << "." 
		 << std::setfill('0') << std::setw(4) << instance_count << "."
		 << std::setfill('0') << std::setw(4) << j << " "; 
	file_out << ctx0 << std::endl;
	*/

	// Predict 5 words we could be typing at the moment.
	// From these 5, we predict more words with ibase1.
	//
	std::vector<std::string> strs0;
	std::string t0;

	generate_tree( My_Experiment0, ctx0, strs0, length0, depths0, t0 );

	for ( int s = 0; s < strs0.size(); s++ ) {
	  std::string lpred = strs0.at(s).substr(1, strs0.at(s).length()-1);
	  //l.log( "pred="+to_str(s)+"/"+lpred );
	  
	  if ( lpred == token ) { //NB these spaces in the beginning
	    //l.log( "MATCH INSIDE WORD" );
      
	    lsaved = letters.size() - j - 1; // NB, we subtract the space also

	    // So, if it is the last word, we substract one.
	    //
	    if ( i == words.size()-1 ) {
	      --lsaved; // could become 0 again?
	    }

	    // L0000.0000.0004 o m m u communication 8
	    //
	    file_out << "L" << std::setfill('0') << std::setw(4) << sentence_count << "." 
		     << std::setfill('0') << std::setw(4) << instance_count << "."
		     << std::setfill('0') << std::setw(4) << j << " "; 
	    file_out << ctx0 << " " << lpred << " " << lsaved << std::endl;
	    
	    /*lout = "L" + to_str((double)sentence_count, 4) + "." + to_str((double)instance_count, 4) + "."
	      + to_str(j, 4) + " " + ctx0.toString() + " " + lpred + " ";*/
	      
	    // The pred. should be insterted in this context here, if match inside word.
	    // actually, the lpred is what is already in ctx1, because if we match it is
	    // the current word.
	    // We "just" need to know how many extra letters we saved
	    //
	    Context ctx01 = ctx1;
	    //l.log( "ctx1="+ctx1.toString() );
	    //l.log( "ctx01="+ctx01.toString() );

	    // Continue with words based on correctly guessed 
	    // letter continuation.
	    //
	    generate_tree( My_Experiment1, ctx01, strs, length, depths, t );

	    // We need to shift our ctx0 with the predicted letters.
	    //
	    for ( int j1 = j+1; j1 < letters.size(); j1++ ) {
	      letter = letters.at(j1);
	      ctx0.push( letter );
	    }

	    // Break? We only need to predict our word once.
	    // break outerloop also
	    //
	    break;
	  } // lpred == token
	  if ( lsaved > 0 ) {
	    break;
	  }
	} // s over strs0
	if ( lsaved > 0 ) {
	  break;
	}
      } // j over letters
      // End of word.
      letters.clear();
      ctx0.push( "_" );

      //l.log( "IBASE1 predictions: "+to_str(strs.size()) );

      // If lsaved == 0, we call 
      // generate_tree( My_Experiment1, c, ctx01, strs, length, depths, t );
      // again. It means we never completed the word correctly, and are now
      // at a " ". At this point, we run the word predictor.
      //
      if ( lsaved == 0 ) {
	//l.log( "Adding predictions." );
	//l.log( "ctx1="+ctx1.toString() );
	generate_tree( My_Experiment1, ctx1, strs, length, depths, t );
	//l.log( "IBASE1 predictions: "+to_str(strs.size()) );
      }

      // Print the instance, with counts.
      // I0000.0004 waited for
      //
      /*
      file_out << "I" << std::setfill('0') << std::setw(4) << sentence_count << "." 
	       << std::setfill('0') << std::setw(4) << instance_count << " "; 
      file_out << ctx1 << std::endl;
      */

      std::vector<std::string>::iterator si = strs.begin();
      prediction_count = 0;
      long savedhere = 0; // key presses saved in this prediction.
      int adjust = 0; // Adjust the keys saved if we find word predictor matches.
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
	  savedhere = matched.size()-1; // -1 for space.
	  sentenceksaved += savedhere;
	  sentencewsaved += words_matched;
	}

	// P0000.0004.0004 his sake and
	//
	// (print only when a match, optional?)
	//
	// The lsaved can be added here, or counted seperately.
	//
	if ( ( matchesonly && (matched != "") ) || ( matchesonly == false ) ) {
	  file_out << "P" << std::setfill('0') << std::setw(4) << sentence_count << "." 
		   << std::setfill('0') << std::setw(4) << instance_count << "." 
		   << std::setfill('0') << std::setw(4) << prediction_count << (*si) << std::endl;
	  
	  if ( matched != "" ) {
	    file_out << "M" << std::setfill('0') << std::setw(4) << sentence_count << "." 
		     << std::setfill('0') << std::setw(4) << instance_count << "." 
		     << std::setfill('0') << std::setw(4) << prediction_count 
		     << " " << matched << matched.size()-1 << std::endl; // -1 for trailing space. 
	    
	    // We need to add one for the space after the letter predictor, that
	    // is already subtracted.
	    //
	    adjust = 1;
	  }
	}
	prediction_count++;
	si++;
      } //si over strs

      strs.clear();
      
      // If we had word matches, we win the space after the letter 
      // classifier.
      //
      if ( (lsaved > 0) && (adjust > 0) ) {
	lsaved = lsaved + adjust;
      }
      
      keyssaved += savedhere;
      letterssaved += lsaved;
      sletterssaved += lsaved;
      
      ++instance_count;
    } // i over words

    // Output Result for this sentence.
    //
    file_out << "R" << std::setfill('0') << std::setw(4) << sentence_count << " "; 
    file_out << /*a_line << " " <<*/ sentencewsaved << " " << sentenceksaved << " ";
    file_out << sletterssaved << std::endl;

    ctx0.reset(); // leave out to leave previous sentence in context
    ctx1.reset();

    ++sentence_count;
  }

  // Output Totals
  //
  file_out << "T0 " << keypresses << " " << letterssaved << " " << ((double)letterssaved/keypresses)*100.0 << std::endl;
  file_out << "T1 " << keypresses << " " << keyssaved << " " << ((double)keyssaved/keypresses)*100.0 << std::endl;
  file_out << "T " << keypresses << " " << (keyssaved+letterssaved) << " " << ((double)(keyssaved+letterssaved)/keypresses)*100.0 << std::endl;

  l.log( "Keypresses: " + to_str(keypresses) );
  l.log( "Letters   : " + to_str(letterssaved) );
  l.log( "Saved     : " + to_str(keyssaved) );

  c.add_kv( "pdt2_file", output_filename );
  l.log( "SET pdt2_file to "+output_filename );

  return 0;
}  


// Should implement caching? Threading, ...
// Cache must be 'global', contained in c maybe, or a parameter.
//
void generate_next( Timbl::TimblAPI* My_Experiment, std::string instance, std::vector<distr_elem>& distr_vec ) {

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
void generate_tree( Timbl::TimblAPI* My_Experiment, Context& ctx, std::vector<std::string>& strs, int length, std::vector<int>& depths, std::string t ) {

  if ( length == 0 ) {
    // here we should save/print the whole "string" (?)
    //std::cout << "STRING: [" << t << "]" << std::endl;
    strs.push_back( t );
    return;
  }

  //std::cerr << "gen_tree: ctx=" << ctx.toString() << " / l=" << length << ", t=" << t << std::endl;

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
  generate_next( My_Experiment, instance, res );

  //std::cerr << "RESSIZE=" << res.size() << std::endl;
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

    // NB the extra space in the beginning.
    //
    generate_tree( My_Experiment, new_ctx, strs, length-1, depths, t+" "+(*fi).name );//+"/"+to_str(res.size()) );
    /*if ( t.length() > 0 ) {
      generate_tree( My_Experiment, new_ctx, strs, length-1, depths, t+" "+(*fi).name );//+"/"+to_str(res.size()) );
    } else {
      generate_tree( My_Experiment, new_ctx, strs, length-1, depths, (*fi).name );//+"/"+to_str(res.size()) );
      }*/
    
    fi++;
  }
  
}

// --

// foo -> f o o
//
int explode(std::string a_word, std::vector<std::string>& res) {
  for ( int i = 0; i < a_word.length(); i++ ) {
    std::string tmp = a_word.substr(i, 1);
    res.push_back( tmp );
  }
  return res.size();
}

// Generates letter instances. Example for lc:2, "The":
// _ T  The
// T h  The
// h e  _
//
void window_word_letters(std::string a_word, std::string t, int lc, Context& ctx, std::vector<std::string>& res) {
  int i;
  for ( i = 0; i < a_word.length()-1; i++ ) { //NB ()-1
    std::string tmp = a_word.substr(i, 1);
    ctx.push( tmp ); // next letter in context
    res.push_back( ctx.toString() + " " + t );
  }
  // After the last letter, predict a space instead.
  // Do we want to predict spaces? Just skip this instance?
  std::string tmp = a_word.substr(i, 1);
  ctx.push( tmp ); // next letter in context
  res.push_back( ctx.toString() + " " + "_" ); //instead of t
}

// Loop over one sentence.
// "The man", lc:2
// _ T  The
// T h  The
// h e  _   <- from window_word_letters(...)
// e _ man  <- from extra space inserted
// _ m man
// 
void window_words_letters(std::string a_line, int lc, Context& ctx, std::vector<std::string>& res) {
  std::vector<std::string> words;
  std::vector<std::string>::iterator wi;
  Tokenize( a_line, words );
  int i;
  for ( i = 0; i < words.size(); i++ ) { // each word
    // If not the first word, insert a "space" before the words (or after the previous).
    // Also, redict the "current" word after the space.
    if ( i > 0 ) {
      ctx.push( "_" );
      res.push_back( ctx.toString() + " " + words.at(i)); 
    }
    // Continue with the individual letters of the word.
    window_word_letters( words.at(i), words.at(i), lc, ctx, res );
  }
}

// Window a whole text letter based. Treat text as one long stream of words
// to be able to predict next word after a "." &c.
//
int window_letters( Logfile& l, Config& c ) {
  l.log( "window_letters" );
  const std::string& filename        = c.get_value( "filename" );
  int                lc              = stoi( c.get_value( "lc", "2" ));
  int                rc              = 0;
  int                mode            = stoi( c.get_value( "lm", "o" )); // lettering mode 0=shift, 1=empty for new sentence

  if ( (mode < 0) || (mode > 1) ) {
    l.log( "NOTICE: wrong lm parameter, setting to 0." );
    mode = 0;
  }
  std::string        output_filename = filename + ".lt" + to_str(lc)+"m"+to_str(mode); // RC makes no sense (yet)

  l.inc_prefix();
  l.log( "filename:  "+filename );
  l.log( "lc:        "+to_str(lc) );
  l.log( "lm:        "+to_str(mode) );
  l.log( "OUTPUT:    "+output_filename );
  l.dec_prefix();

  if ( file_exists( l, c, output_filename )) {
    l.log( "OUTPUT exists, not overwriting." );
    c.add_kv( "filename", output_filename );
    l.log( "SET filename to "+output_filename );
    return 0;
  }
 
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

  std::string                        a_word;
  std::string                        a_line;
  bool                               first = true; //false to get first line with empty context
  std::vector<std::string>           results;
  std::vector<std::string>::iterator ri;
  Context ctx(lc);
  
  if ( mode == 0 ) {
    while ( file_in >> a_word ) { 

      if ( ! first ) {
	//window_word_letters( "_", a_word, lc, ctx, results );
	// We start by pushin a _ in the context and predicting the current
	// word. We continue by windowing the rest of the current word
	// normally. The last windowed prediction of each word predicts
	// a _, therefore we need to push it in here.
	ctx.push( "_" );
	results.push_back( ctx.toString() + " " + a_word ); 
      }
      window_word_letters(a_word, a_word, lc, ctx, results);
      for ( ri = results.begin(); ri != results.end(); ri++ ) {
	file_out << *ri << "\n";
      }
      first =  false;
      results.clear();
    }
  } else if ( mode == 1 ) {

    while( std::getline( file_in, a_line ) ) { 
      if ( a_line == "" ) {
	continue;
      }
      window_words_letters(a_line, lc, ctx, results);
      for ( ri = results.begin(); ri != results.end(); ri++ ) {
	file_out << *ri << "\n";
      }
      results.clear();
      ctx.reset();

    } // while
  }


  file_out.close();
  file_in.close();
  
  c.add_kv( "filename", output_filename );
  l.log( "SET filename to "+output_filename );
  return 0;
}

#else
int pdt( Logfile& l, Config& c ) {
  l.log( "No TIMBL support." );
  return -1;
}  
int pdt2( Logfile& l, Config& c ) {
  l.log( "No TIMBL support." );
  return -1;
}  
#endif

/*
  Server/web demo
  
  implement the following:
  1) init (inits the contexts till empty).
  2) type:x (adds ('types') letter x, add it to the letter context)
     (this should trigger completion &c. Should it return it?)
  3) get (get latest completion (stored in 2)
  4) select:n (select one of the completions presented. "skip:n,m")

  *) global server data structure. 2 contexts, settings, timbl classifiers.
*/

// code to make a PDT

int pe_init( PDT& pdt ) {
  //pdt.reset(); //resets contexts
}

int pe_type( PDT& pdt, std::string& l ) {
  //pdt.ltr_ctx.push( l );
}

int pe_complete( PDT& pdt ) {
  // like pdt2, maybe delegate to PDT.complete()
  //generate_tree( My_Experiment0, c, ctx0, strs0, length0, depths0, t0 );
}

#include "tinyxml.h"

int pdt2web( Logfile& l, Config& c ) {
  l.log( "work in progress pdt2web" );

  const std::string port        = c.get_value( "port", "1984" );
  const int verbose             = stoi( c.get_value( "verbose", "1" ));
  const int keep                = stoi( c.get_value( "keep", "0" ));

  const std::string& ibasefile0      = c.get_value( "ibasefile0" );
  const std::string& timbl0          = c.get_value( "timbl0" );
  const std::string& ibasefile1      = c.get_value( "ibasefile1" );
  const std::string& timbl1          = c.get_value( "timbl1" );
  int                lc0             = stoi( c.get_value( "lc0", "2" ));
  int                rc0             = stoi( c.get_value( "rc0", "0" )); // should be 0
  int                lc1             = stoi( c.get_value( "lc1", "2" ));
  int                rc1             = stoi( c.get_value( "rc1", "0" )); // should be 0
  std::string        ped             = c.get_value( "ds", "" ); // depths
  int                pel             = stoi( c.get_value( "n", "3" )); // length. implicit in ds!
  std::string        dl              = c.get_value( "dl", "3" ); // letter depth 
  int                mlm             = stoi( c.get_value( "mlm", "0" )); // minimum letter match

  PDTC pdtc0;
  pdtc0.init( ibasefile0, timbl0, lc0 );
  l.log( "pdtc0.status = "+to_str( pdtc0.status ) );

  PDTC pdtc1;
  pdtc1.init( ibasefile1, timbl1, lc1 );
  l.log( "pdtc1.status = "+to_str( pdtc1.status ) );

  PDT pdt;
  pdt.set_ltr_c( &pdtc0 );
  pdt.set_wrd_c( &pdtc1 );

  pdt.set_wrd_ds( ped );
  pdt.set_ltr_dl( dl );

  // --


  std::vector<std::string> strs;
  std::vector<std::string>::iterator si = strs.begin();
  std::vector<std::string> letters;
  std::vector<std::string>::iterator li;
  letters.clear();

  /*
  std::string token = "hallo";
  std::string foo;
  (void)explode( token, letters );
  for ( int j = 0; j < letters.size(); j++ ) {

    // spaces special treatment.

    pdt.add_ltr( letters.at(j) );
    strs.clear();
    pdt.ltr_generate( strs );
    si = strs.begin();
    while ( si != strs.end() ) {
      std::string s = *si;
      s = s.substr( 1, s.length()-1);
      std::cerr << "(" << s << ")" << std::endl;
      Context *old = new Context(pdt.wrd_ctx); //save
      pdt.add_wrd( s );
      std::vector<std::string> strs_wrd;
      std::vector<std::string>::iterator si_wrd;
      pdt.wrd_generate( strs_wrd );
      si_wrd = strs_wrd.begin();
      while ( si_wrd != strs_wrd.end() ) {
	std::cerr << *si_wrd << std::endl;
	++si_wrd;
      }
      delete pdt.wrd_ctx;
      pdt.wrd_ctx = old; // restore.
      ++si;
    }
    std::cerr << std::endl;
  }

  foo = "o";
  pdt.add_ltr( foo );
  strs.clear();
  pdt.ltr_generate( strs );
  si = strs.begin();
  while ( si != strs.end() ) {
    std::string s = *si;
    s = s.substr( 1, s.length()-1);
    std::cerr << "(" << s << ")" << std::endl;
    Context *old = new Context(pdt.wrd_ctx); //save
    pdt.add_wrd( s );
    std::vector<std::string> strs_wrd;
    std::vector<std::string>::iterator si_wrd;
    pdt.wrd_generate( strs_wrd );
    si_wrd = strs_wrd.begin();
    while ( si_wrd != strs_wrd.end() ) {
      std::cerr << *si_wrd << std::endl;
      ++si_wrd;
    }
    delete pdt.wrd_ctx;
    pdt.wrd_ctx = old; // restore.
    ++si;
  }
  */

  /*
  strs.clear();
  pdt.wrd_generate( strs );
  si = strs.begin();
  while ( si != strs.end() ) {
    std::cerr << *si << std::endl;
    ++si;
    }*/


  Sockets::ServerSocket server;
  
  if ( ! server.connect( port )) {
    l.log( "ERROR: cannot start server: "+server.getMessage() );
    return 1;
  }
  if ( ! server.listen(  ) < 0 ) {
    l.log( "ERROR: cannot listen. ");
    return 1;
  };
    
  bool run = true;
  std::vector<std::string> buf_tokens;
  while ( run ) {  // main accept() loop

    l.log( "Listening..." );  
    
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

    std::string buf;
    newSock->read( buf );
    buf = trim( buf, " \n\r" );
    l.log( "("+buf+")" );

    if ( buf == "QUIT" ) {
      //
      // Quit...
      //
      run = false;
    } else if ( buf == "GEN" ) { 
      //
      // Generate from the contexts. First complete with the letter predictor,
      // then follow up with the word predictor.
      //
      TiXmlDocument doc;
      TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "", "" );
      TiXmlElement * element = new TiXmlElement( "result" );
      doc.LinkEndChild( decl );
      doc.LinkEndChild( element );

      strs.clear();
      pdt.ltr_generate( strs );
      si = strs.begin();
      int cnt = 0;
      while ( si != strs.end() ) {
	std::string s = *si;
	s = s.substr( 1, s.length()-1);
	Context *old = new Context(pdt.wrd_ctx); //save
	pdt.add_wrd( s );
	std::vector<std::string> strs_wrd;
	std::vector<std::string>::iterator si_wrd;
	pdt.wrd_generate( strs_wrd );
	si_wrd = strs_wrd.begin();
	while ( si_wrd != strs_wrd.end() ) {

	  TiXmlElement * element0 = new TiXmlElement( "gen" );
	  element0->SetAttribute("idx", cnt++);
	  TiXmlText * text0 = new TiXmlText( s + *si_wrd );
	  element0->LinkEndChild( text0 );
	  element->LinkEndChild( element0 );

	  //l.log( s + *si_wrd );
	  ++si_wrd;
	}
	delete pdt.wrd_ctx;
	pdt.wrd_ctx = old; // restore.
	++si;
      }
      std::ostringstream ostr;
      ostr << doc;
      newSock->write( ostr.str() );
      l.log( ostr.str() );
    } else if ( buf == "CTX" ) {
      //
      // Returns the two contexts.
      //
      l.log( pdt.ltr_ctx->toString() );
      l.log( pdt.wrd_ctx->toString() );

      TiXmlDocument doc;
      TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "", "" );

      TiXmlElement * element0 = new TiXmlElement( "ltr" );
      TiXmlText * text0 = new TiXmlText( pdt.ltr_ctx->toString() );
      element0->LinkEndChild( text0 );
      TiXmlElement * element1 = new TiXmlElement( "wrd" );
      TiXmlText * text1 = new TiXmlText( pdt.wrd_ctx->toString() );
      element1->LinkEndChild( text1 );

      TiXmlElement * element = new TiXmlElement( "result" );
      element->LinkEndChild( element0 );
      element->LinkEndChild( element1 );
      doc.LinkEndChild( decl );
      doc.LinkEndChild( element );

      std::ostringstream ostr;
      ostr << doc;

      std::string answer = pdt.wrd_ctx->toString() + '\n';
      newSock->write( ostr.str() );
      l.log( ostr.str() );
    } else if ( buf == "SPC" ) {
      pdt.add_spc();      
    }

    buf_tokens.clear();
    Tokenize( buf, buf_tokens, ' ' );

    if ( buf_tokens.size() > 1 ) {

      if ( buf_tokens.at(0) == "LTR" ) {
	//
	// Add one letter to the letter context.
	//
	pdt.add_ltr( buf_tokens.at(1) );      
      } else if ( buf_tokens.at(0) == "WRD" ) {
	//
	// Add a word to the word context. Explode the word,
	// and add each letter to the letter context as well.
	//
	pdt.add_wrd( buf_tokens.at(1) );

	std::vector<std::string> letters;
	std::vector<std::string>::iterator li;
	(void)explode( buf_tokens.at(1), letters );
	// need a space before?
	for ( int j = 0; j < letters.size(); j++ ) {
	  pdt.add_ltr( letters.at(j) );
	}
      } else if ( buf_tokens.at(0) == "GEN" ) {
	//
	// Seperate generators for words and letters.
	//
	if ( buf_tokens.at(1) == "WRD" ) {
	  strs.clear();
	  std::vector<std::string> strs_wrd;
	  std::vector<std::string>::iterator si_wrd;
	  pdt.wrd_generate( strs_wrd );
	  si_wrd = strs_wrd.begin();
	  while ( si_wrd != strs_wrd.end() ) {
	    l.log( *si_wrd );
	    ++si_wrd;
	  }
	} else if ( buf_tokens.at(1) == "LTR" ) {
	  strs.clear();
	  pdt.ltr_generate( strs );
	  si = strs.begin();
	  while ( si != strs.end() ) {
	    l.log( *si );
	    ++si;
	  }
	}
	
      }

    }

    l.log( "connection closed." );
    delete newSock;
    
  } // while true

  return 0;
}
