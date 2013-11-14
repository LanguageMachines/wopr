#include <string>
#include "wopr/Classifier.h"

Classifier::Classifier( const std::string& n ) {
  id      = n;
  type    = 1; // Timbl
  subtype = 0;
}

Classifier::Classifier( const std::string& n, int t ) {
  id      = n;
  type    = t;
  subtype = 0;
}
