#include <string>
#include "Classifier.h"

Classifier::Classifier( const std::string& n ) {
  id   = n;
  type = 1; // Timbl
}

Classifier::Classifier( const std::string& n, int t ) {
  id   = n;
  type = t;
}
