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
#include "tag.h"
#include "Classifier.h"
#include "Multi.h"
#include "levenshtein.h"

#ifdef TIMBL
# include "timbl/TimblAPI.h"
#endif

int min3( int a, int b, int c ) {
  int mi;
  
  mi = a;
  if (b < mi) {
    mi = b;
  }
  if (c < mi) {
    mi = c;
  }
  return mi;  
}


int lev_distance(const std::string source, const std::string target) {

  // Step 1

  const int n = source.length();
  const int m = target.length();
  if (n == 0) {
    return m;
  }
  if (m == 0) {
    return n;
  }

  typedef std::vector< std::vector<int> > Tmatrix; 
  Tmatrix matrix(n+1);

  // Size the vectors in the 2.nd dimension. Unfortunately C++ doesn't
  // allow for allocation on declaration of 2.nd dimension of vec of vec

  for (int i = 0; i <= n; i++) {
    matrix[i].resize(m+1);
  }

  // Step 2

  for (int i = 0; i <= n; i++) {
    matrix[i][0]=i;
  }

  for (int j = 0; j <= m; j++) {
    matrix[0][j]=j;
  }

  // Step 3

  for (int i = 1; i <= n; i++) {

    const char s_i = source[i-1];

    // Step 4

    for (int j = 1; j <= m; j++) {

      const char t_j = target[j-1];

      // Step 5

      int cost;
      if (s_i == t_j) {
        cost = 0;
      } else {
        cost = 1;
      }

      // Step 6

      int above = matrix[i-1][j];
      int left  = matrix[i][j-1];
      int diag  = matrix[i-1][j-1];
      //const int cell  = min( above + 1, min(left + 1, diag + cost));
      int cell  = min3( above + 1, left + 1, diag + cost );

      // Step 6A: Cover transposition, in addition to deletion,
      // insertion and substitution. This step is taken from:
      // Berghel, Hal ; Roach, David : "An Extension of Ukkonen's 
      // Enhanced Dynamic Programming ASM Algorithm"
      // (http://www.acm.org/~hlb/publications/asm/asm.html)

      if ( i>2 && j>2 ) {
        int trans = matrix[i-2][j-2] + 1;
        if ( source[i-2] != t_j ) {
	  trans++;
	}
        if ( s_i != target[j-2] ) {
	  trans++;
	}
        if ( cell > trans ) {
	  cell = trans;
	}
      }

      matrix[i][j] = cell;
    }
  }

  // Step 7

  return matrix[n][m];
}

int levenshtein( Logfile& l, Config& c ) {

  l.log( to_str( lev_distance( "gumbo", "gambol" )));
  l.log( to_str( lev_distance( "peter", "petr" )));

}

// Spellings correction.
//
/*
1. Meenemen van lexicale frequenties in Wopr Correct. We hadden het daar
al over; je neemt de frequenties mee uit het trainingset-lexicon en
geeft die mee in de output - of doet er zelfs al filtering op. Daarnaast
vraag ik me af of naast OF in plaats van een frequentiedrempel, er ook
een drempel gezet kan worden op de distributiegrootte of de entropie.
Dus: als entropy > 5, of als distributie > 100, of als frequentie-ratio
> 100, onderdruk dan de gesuggereerde correctie. In feite heb je hiermee
drie parameters. Zou je eens kunnen bedenken of dit in te bouwen is? 
*/
#ifdef TIMBL
struct distr_elem {
  std::string name;
  double      freq;
  bool operator<(const distr_elem& rhs) const {
    return freq > rhs.freq;
  }
};
int correct( Logfile& l, Config& c ) {
  l.log( "pplx" );
  const std::string& filename         = c.get_value( "filename" );
  const std::string& ibasefile        = c.get_value( "ibasefile" );
  const std::string& lexicon_filename = c.get_value( "lexicon" );
  const std::string& counts_filename  = c.get_value( "counts" );
  const std::string& timbl            = c.get_value( "timbl" );
  bool               to_lower         = stoi( c.get_value( "lc", "0" )) == 1;
  std::string        output_filename  = filename + ".sc";
  std::string        pre_s            = c.get_value( "pre_s", "<s>" );
  std::string        suf_s            = c.get_value( "suf_s", "</s>" );
  // minimum word length
  int                mwl              = stoi( c.get_value( "mwl", "5" ) );
  // maximum levenshtein distance
  int                mld              = stoi( c.get_value( "mld", "1" ) );
  // max entropy
  int                max_ent          = stoi( c.get_value( "max_ent", "5" ) );
  // maximum distributie
  int                max_distr        = stoi( c.get_value( "max_distr", "10" ));
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
  l.log( "counts:     "+counts_filename );
  l.log( "timbl:      "+timbl );
  l.log( "lowercase:  "+to_str(to_lower) );
  l.log( "mwl:        "+to_str(mwl) );
  l.log( "mld:        "+to_str(mld) );
  l.log( "max_ent:    "+to_str(max_ent) );
  l.log( "max_distr:  "+to_str(max_distr) );
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
  //l.log( "P(new_particular) = " + to_str(p0) );

  try {
    My_Experiment = new Timbl::TimblAPI( timbl );
    (void)My_Experiment->GetInstanceBase( ibasefile );
    // My_Experiment->Classify( cl, result, distrib, distance );
  } catch (...) {
    l.log( "Cannot create Timbl Experiment..." );
    return 1;
  }

  std::vector<std::string>::iterator vi;
  std::ostream_iterator<std::string> output( file_out, " " );

  std::string a_line;
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
  
  // Recognise <s> or similar, reset pplx calculations.
  // Output results on </s> or similar.
  // Or a divisor which is not processed?
  //
  double sentence_prob      = 0.0;
  double sum_logprob        = 0.0;
  int    sentence_wordcount = 0;

  while( std::getline( file_in, a_line )) {

    if ( to_lower ) {
      std::transform( a_line.begin(),a_line.end(),a_line.begin(),tolower ); 
    }

    words.clear();
    a_line = trim( a_line );
    Tokenize( a_line, words, ' ' );
    if ( words.size() == 1 ) { // For Hermans data. TODO: better fix.
      words.clear();
      Tokenize( a_line, words, '\t' );
    }
    std::string target = words.at( words.size()-1 );
    
    ++sentence_wordcount;

    // Is the target in the lexicon? We could calculate a smoothed
    // value here if we load the .cnt file too...
    //
    std::map<std::string,int>::iterator wfi = wfreqs.find( target );
    bool   target_unknown = false;
    bool   correct_answer = false;
    double target_lexfreq = 0.0;// should be double because smoothing
    double target_lexprob = 0.0;
    if ( wfi == wfreqs.end() ) {
      target_unknown = true;
    } else {
      target_lexfreq =  (int)(*wfi).second; // Take lexfreq, unless we smooth
      std::map<int,double>::iterator cfi = c_stars.find( target_lexfreq );
      if ( cfi != c_stars.end() ) { // We have a smoothed value, use it
	target_lexfreq = (double)(*cfi).second;
	//l.log( "smoothed_lexfreq = " + to_str(target_lexfreq) );
      }
      target_lexprob = (double)target_lexfreq / (double)total_count;
    }

    // What does Timbl think?
    // Do we change this answer to what is in the distr. (if it is?)
    //
    tv = My_Experiment->Classify( a_line, vd );
    std::string answer = tv->Name();
    //l.log( "Answer: '" + answer + "' / '" + target + "'" );
    
    if ( target == answer ) {
      ++correct;
      correct_answer = true;
    } else {
      ++wrong;
    }

    // Loop over distribution returned by Timbl.
    /*
      Je hebt een WOPR, en je laat dat los op een nieuw stuk tekst. Voor ieder
      woord in de tekst kijk je of het in de distributie zit. Zo NIET, dan
      check je of er een woord in de distributie zit dat op een kleine
      Levenshtein-afstand van het woord staat, bijvoorbeeld op afstand 1,
      dus er is 1 insertie/deletie/transpositie verschil tussen de twee
      woorden.
    */
    // For spelling correction we look in the distro. If the word is not
    // there, we look at levenshtein distance 1, if there is a word
    // it could be the 'correction' of our target word.
    //
    // PJB: patterns containing spelling errors....
    //
    //
    Timbl::ValueDistribution::dist_iterator it = vd->begin();
    int cnt = 0;
    int distr_count = 0;
    int target_freq = 0;
    int answer_freq = 0;
    double prob            = 0.0;
    double target_distprob = 0.0;
    double answer_prob     = 0.0;
    double entropy         = 0.0;
    bool in_distr          = false;
    cnt = vd->size();
    distr_count = vd->totalSize();

    std::vector<distr_elem> distr_vec;

    // Check if target word is in the distribution.
    //
    while ( it != vd->end() ) {
      //const Timbl::TargetValue *tv = it->second->Value();

      std::string tvs  = it->second->Value()->Name();
      double      wght = it->second->Weight();

      // Prob. of this item in distribution.
      //
      prob     = (double)wght / (double)distr_count;
      entropy -= ( prob * log2(prob) );

      if ( tvs == target ) { // The correct answer was in the distribution!
	target_freq = wght;
	in_distr = true;
	if ( correct_answer == false ) {
	  ++correct_distr;
	  --wrong; // direct answer wrong, but right in distr. compensate count
	}
      }

      ++it;
    }
    target_distprob = (double)target_freq / (double)distr_count;

    // If correct: if target in distr, we take that prob, else
    // the lexical prob.
    // Unknown words?
    //
    double logprob = 0.0;
    std::string info = "huh?";
    if ( target_freq > 0 ) { // Right answer was in distr.
      logprob = log2( target_distprob );
      info = "target_distprob";
    } else {
      if ( ! target_unknown ) { // Wrong, we take lex prob if known target
	logprob = log2( target_lexprob ); // SMOOTHED here, see above
	info = "target_lexprob";
      } else {
	//
	// What to do here? We have an 'unknown' target, i.e. not in the
	// lexicon.
	//
	logprob = log2( p0 /*0.0001*/ ); // Foei!
	info = "P(new_particular)";
      }
    }
    sum_logprob += logprob;

    // I we didn't have the correct answer in the distro, we take ld=1
    // Skip words shorter than mwl.
    //
    it = vd->begin();
    if ( (target.length() > mwl) && (in_distr == false) ) {
      while ( it != vd->end() ) {
	
	std::string tvs  = it->second->Value()->Name();
	double      wght = it->second->Weight();
	
	int ld = lev_distance( target, tvs );
	
	// If the ld of the word is less than the minimum,
	// we include the result in our output.
	//
	if (
	    (entropy <= max_ent) &&
	    (cnt <= max_distr) &&
	    ( ld <= mld ) 
	    ) { 
	    distr_elem  d;
	    d.name = tvs;
	    d.freq = ld;
	    distr_vec.push_back( d );
	}
	
	++it;
      }
    }

    // Word logprob (ref. Antal's mail 21/11/08)
    // 2 ^ (-logprob(w)) 
    //
    double word_lp = pow( 2, -logprob );

    // What do we want in the output file? Write the pattern and answer,
    // the logprob, followed by the entropy (of distr.), the size of the
    // distribution returned, and the top-10 (or less) of the distribution.
    //
    file_out << a_line << " (" << answer << ") "
	     << logprob << ' ' /*<< info << ' '*/ << entropy << ' ';
    file_out << word_lp << ' ';

    int cntr = 0;
    sort( distr_vec.begin(), distr_vec.end() );
    std::vector<distr_elem>::iterator fi;
    fi = distr_vec.begin();
    file_out << cnt << " [ ";
    while ( (fi != distr_vec.end()) && (--cntr != 0) ) {
      file_out << (*fi).name << ' ' << (*fi).freq << ' ';
      fi++;
    }
    file_out << "]";

    file_out << std::endl;

    // End of sentence (sort of)
    //
    if ( target == "</s>" ) {
      l.log( " sum_logprob = " + to_str( sum_logprob) );
      l.log( " sentence_wordcount = " + to_str( sentence_wordcount ) );
      double foo  = sum_logprob / (double)sentence_wordcount;
      double pplx = pow( 2, -foo ); 
      l.log( " pplx = " + to_str( pplx ) );
      sum_logprob = 0.0;
      sentence_wordcount = 0;
      l.log( "--" );
    }

  } // while getline()

  if ( sentence_wordcount > 0 ) { // Overgebleven zooi (of alles).
    l.log( "sum_logprob = " + to_str( sum_logprob) );
    l.log( "sentence_wordcount = " + to_str( sentence_wordcount ) );
    double foo  = sum_logprob / (double)sentence_wordcount;
    double pplx = pow( 2, -foo ); 
    l.log( "pplx = " + to_str( pplx ) );
  }

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

  return 0;
}
#else
int correct( Logfile& l, Config& c ) {
  l.log( "No TIMBL support." );
  return -1;
}  
#endif
