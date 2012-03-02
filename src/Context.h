#ifndef _CONTEXT_H
#define _CONTEXT_H

#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <map>
#include <vector>
#include <algorithm>
#include <iterator>

class Context {
  std::vector<std::string> data;
  size_t size;
  friend std::ostream& operator << ( std::ostream&, const Context& );

 public:
  Context(size_t);
  Context( const Context& );
  Context( const Context* );
  void push(const std::string&);
  void cp( const Context* );
  const std::string& last_letter();
  std::string toString();
  std::string toString(int);
  void reset();
  size_t get_size();
};

#endif
