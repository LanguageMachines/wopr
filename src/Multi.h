#ifndef _MULTI_H
#define _MULTI_H

#include <string>

class Multi {

 public:
  std::string id;
  std::string classifier;

  //! Constructor.
  //!
  Multi( const std::string& );

  //! Destructor.
  //!
  ~Multi();

  void set_classifier( const std::string c ) {
    classifier = c;
  }

};

#endif

