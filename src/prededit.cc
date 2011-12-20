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
int explode(std::string, std::vector<std::string>&);
void window_word_letters(std::string, std::string, int, Context&, std::vector<std::string>&);
void window_words_letters(std::string, int, Context&, std::vector<std::string>&);

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
      generate_tree( My_Experiment, c, ctx, strs, length, depths, t );

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

  // TEST
  //
  /*
  int test_lc = 8;
  Context test_ctx(test_lc);
  std::string test_word = "Hoewel de politie .";
  std::vector<std::string> test_words;
  Tokenize( test_word, test_words );
  std::vector<std::string> test_res;
  std::vector<std::string>::iterator tri;
  window_words_letters(test_word, test_lc, test_ctx, test_res);
  for ( tri = test_res.begin(); tri != test_res.end(); tri++ ) {
    l.log(*tri);
  }
  return 0;
  */
  //
  // END TEST


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

	// Predict 5 words we could be typing at the moment.
	// From these 5, we predict more words with ibase1.
	//
	std::vector<std::string> strs0;
	std::string t0;

	generate_tree( My_Experiment0, c, ctx0, strs0, length0, depths0, t0 );

	for ( int s = 0; s < strs0.size(); s++ ) {
	  std::string lpred = strs0.at(s).substr(1, strs0.at(s).length()-1);
	  //l.log( "pred="+to_str(s)+"/"+lpred );
	  
	  if ( lpred == token ) { //NB these spaces in the beginning
	    //l.log( "MATCH INSIDE WORD" );
      
	    lsaved = letters.size()-j; // NB, this includes the space after the word.

	    file_out << "L" << std::setfill('0') << std::setw(4) << sentence_count << "." 
		     << std::setfill('0') << std::setw(4) << instance_count << " "; 
	    file_out << ctx0 << " " << lpred << " " << j << "/" << letters.size() << "=" << lsaved << std::endl;

	    // The pred. should be insterted in this context here, if match inside word.
	    // actually, the lpred is what is already in ctx1, because if we match it is
	    // the current word.
	    // We "just" need to know how many extra letters we saved
	    //
	    Context ctx01 = ctx1;
	    //l.log( "ctx1="+ctx1.toString() );
	    //l.log( "ctx01="+ctx01.toString() );

	    generate_tree( My_Experiment1, c, ctx01, strs, length, depths, t );

	    // Break? We only need to predict our word once.
	    // break outerloop also
	    //
	    break;
	  }
	  if ( lsaved > 0 ) {
	    break;
	  }
	} // s
	if ( lsaved > 0 ) {
	  break;
	}
      } //letters
      // End of word.
      letters.clear();
      ctx0.push( "_" );

      //l.log( "IBASE1 predictions: "+to_str(strs.size()) );

      // If lsaves == 0, we call 
      // generate_tree( My_Experiment1, c, ctx01, strs, length, depths, t );
      // again?
      if ( lsaved == 0 ) {
	//l.log( "Adding predictions." );
	//l.log( "ctx1="+ctx1.toString() );
	generate_tree( My_Experiment1, c, ctx1, strs, length, depths, t );
	//l.log( "IBASE1 predictions: "+to_str(strs.size()) );
      }

      // Print the instance, with counts.
      // I0000.0004 waited for
      //
      file_out << "I" << std::setfill('0') << std::setw(4) << sentence_count << "." 
	       << std::setfill('0') << std::setw(4) << instance_count << " "; 
      file_out << ctx1 << std::endl;

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
	  savedhere = matched.size()-1; // PLUS lsaved?
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
	}
	}
	prediction_count++;
	si++;
      }
      strs.clear();
      
      keyssaved += savedhere;
      letterssaved += lsaved;
      
      ++instance_count;
    } // i over words

    // Output Result for this sentence.
    //
    file_out << "R" << std::setfill('0') << std::setw(4) << sentence_count << " "; 
    file_out << /*a_line << " " <<*/ sentencewsaved << " " << sentenceksaved << " ";
    file_out << letterssaved << std::endl;

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
    // std::cerr << length << ":" << cnt  << " " << ctx << " -> " << (*fi).name /* << ' ' << (*fi).freq */ << std::endl;

    // Make new context, recurse
    //
    Context new_ctx = Context(ctx);
    new_ctx.push(  (*fi).name );

    // NB the extra space in the beginning.
    //
    generate_tree( My_Experiment, c, new_ctx, strs, length-1, depths, t+" "+(*fi).name );//+"/"+to_str(res.size()) );
    
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

// _ T The
// T h The
// h e The (or: h e _)
//
void window_word_letters(std::string a_word, std::string t, int lc, Context& ctx, std::vector<std::string>& res) {
  int i;
  for ( i = 0; i < a_word.length()-1; i++ ) { //NB ()-1
    std::string tmp = a_word.substr(i, 1);
    ctx.push( tmp ); // next letter in context
    res.push_back( ctx.toString() + " " + t );
  }
  // After the last letter, predict a space instead.
  // Do we want to predict spaces?
  std::string tmp = a_word.substr(i, 1);
  ctx.push( tmp ); // next letter in context
  res.push_back( ctx.toString() + " " + "_" ); //instead of t
}

// Loop over one sentence.
//
void window_words_letters(std::string a_line, int lc, Context& ctx, std::vector<std::string>& res) {
  std::vector<std::string> words;
  std::vector<std::string>::iterator wi;
  Tokenize( a_line, words, ' ' );
  int i;
  for ( i = 0; i < words.size(); i++ ) { // each word
    if ( i > 0 ) {
      ctx.push( "_" );
      res.push_back( ctx.toString() + " " + words.at(i)); 
    }
    window_word_letters( words.at(i), words.at(i), lc, ctx, res );
  }
}

// Window a whoe text letter based. Treat text as one long stream of words
// to be able to predict next word after a "." &c.
//
int window_letters( Logfile& l, Config& c ) {
  l.log( "window_letters" );
  const std::string& filename        = c.get_value( "filename" );
  int                lc              = stoi( c.get_value( "lc", "2" ));
  int                rc              = 0;
  std::string        output_filename = filename + ".lt" + to_str(lc); // RC makes no sense (yet)

  l.inc_prefix();
  l.log( "filename:  "+filename );
  l.log( "lc:        "+to_str(lc) );
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
  bool                               first = true; //false to get first line with empty context
  std::vector<std::string>           results;
  std::vector<std::string>::iterator ri;
  Context ctx(lc);
  
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

