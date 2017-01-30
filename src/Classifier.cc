#include <string>
#include "wopr/Classifier.h"

Classifier::Classifier( const std::string& n ):
  id(n),
  type(1), //Timbl
  subtype(0),
  ws(-1),
  gatepos(-1),
  weight(0.0),
  distprob(0.0),
  correct(-1),
  score_cg(-1),
  score_cd(-1),
  score_ic(-1),
  classification_count(0),
  My_Experiment(0),
  vd(0),
  tv(0)
{
}

Classifier::Classifier( const std::string& n, int t ):
  Classifier(n)
{
  type = t;
}
