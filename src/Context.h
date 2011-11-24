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
  void push(const std::string&);
  std::string toString();
};

inline std::ostream& operator << ( std::ostream& os, const Context& ctx ) {
  size_t i;
  for ( i = 0; i < ctx.size-1; i++ ) {
    os << ctx.data.at( i ) << " ";
  }
  os << ctx.data.at( i );

  return os;
}

Context::Context(size_t s) {
  size = s;
  data.resize( s );
  data.clear();
  for ( size_t i = 0; i < size; i++ ) {
    data.push_back("_");
  }
  //std::cerr << data.size() << std::endl;
}

Context::Context(const Context& old_ctx) {
  size = old_ctx.size;
  data.resize( size );
  data.clear();
  for ( size_t i = 0; i < size; i++ ) {
    data.push_back( old_ctx.data[i] );
  }
}

void Context::push(const std::string& s) {
  if ( data.size() < size ) {
    data.push_back( s );
    //std::cerr << "push: " << data.size() << std::endl;
  } else {
    //std::cerr << "full: " << data.size() << std::endl;
    copy( data.begin()+1, data.end(), data.begin() );
    data.pop_back();
    data.push_back( s );
    //std::cerr << "full: " << data.size() << std::endl;
  }
}

std::string Context::toString() {
  std::string tmp = "";
  size_t i;
  for ( i = 0; i < size-1; i++ ) {
    tmp = tmp + data.at(i) + " ";
  }
  tmp = tmp + data.at(i);
  return tmp;
}
