#ifndef _MULTI_H
#define _MULTI_H

#include <string>

#include "Classifier.h"

class Multi {

 public:
  std::string id;
  std::string classifier; // not here, asnwers
  std::map<Classifier*, std::string> answers; // per classifier.
  std::string answer; // must be array
  std::string target;

  //! Constructor.
  //!
  explicit Multi( const std::string& s): id(s) {
  }

  //! Destructor.
  //!
  ~Multi(){};

  void set_classifier( const std::string& c ) {
    classifier = c;
  }

  void set_target( const std::string& t ) {
    target = t;
  }

  void add_string( const std::string& s ) {
    answer = answer + "/" + s;
  }

  std::string get_answer() {
    return answer;
  }

  void add_answer( Classifier *c, const std::string& a ) {
    answers[c] = a;
  }

  // Return a ref to the map
  //
  std::string get_answers() {
    std::string a = target + ":";
    for ( const auto& aw : answers ) {
      a = a + " " + aw.first->id + ":" + aw.second;
    }

    return a;
  }
};

#endif
