#ifndef _CACHE_H
#define _CACHE_H

#include <list>
#include <string>
#include <map>
#include <memory>
#include <vector>

/*
  In the map, we store the answer, a counter (unused), and an iterator
  to the position in the list. The list keeps how many elements are
  stored in the map, last element is last added. First element is
  the "oldest". We need to store the position in the list in the map
  because list.erase(element) is very slow.
 */
class cache_elem {
 public:
  std::string ans; 
  int cnt;
  std::list<std::string>::iterator listpos;

  cache_elem( const std::string& s ) {
    ans = s;
    cnt = 1;
  };
};

class Cache {

 private:
  int cache_size;
  std::string empty;
  long hits;
  long stores;
  long misses;
  long reinserts;
  long evictions;

 public:
  std::map<std::string, cache_elem*> cache;
  std::map<std::string, cache_elem*>::iterator ci;
  std::list<std::string> cache_list;
  std::list<std::string>::iterator cli;

  //! Constructor.
  //!
  Cache() {
    cache_size = 10;
    empty = "";
    hits = misses = evictions = stores = reinserts = 0;
  };
  Cache( int s ) {
    if ( s < 0 ) {
      s = 0;
    }
    cache_size =  s;
    empty = "";
    hits = misses = evictions = stores = reinserts = 0;
  };

  //! Destructor.
  //!
  ~Cache() {
    // implement
  };

  void set_size( const int s ) {
    if ( s <= 0 ) {
      return;
    }
    cache_size = s;
  }
  size_t get_size() {
    return cache.size();
  }

  // Add an element to the cache. First we check if the cache is full.
  // If it is not, we store the element in the cache, and add an
  // entry to the end of the list. Update the cache with the element
  // position. We check if the element was already in the map - if it
  // was, we move its entry to the back of the list again (restrengthen
  // it). We do not store it again in the map, so the value cannot be
  // updated.
  // If it is, we remove the element at the front of the list from
  // both the map and the list.
  //
  void add( const std::string& a, const std::string& b ) {

    if ( cache_size == 0 ) {
      return; // fall through for size-0
    }

    ++stores;
	
    int cs = cache.size();
    if ( cs < cache_size ) {
      // if contains, we need to update.
      //
      cache_elem *ce = contains( a );
      if ( ce != NULL ) {
	++reinserts;
	cli = ce->listpos;                  // pos of element in list
	cache_list.erase( cli );            // erase from list
	cache_list.push_back( a );          // reinsert at the back
	ce->listpos = --(cache_list.end()); // update pointer
	++reinserts;
      } else {
	cache_elem *ce = new cache_elem( b ); // create new element
	cache[ a ] = ce;
	cache_list.push_back( a );            // insert in the cache
	ce->listpos = --(cache_list.end());   // update listpos
      }
    } else {
      std::string to_remove = cache_list.front(); // oldest element
      cache_elem *ce = contains( to_remove );     // the element
      delete ce;                                  // be gone
      cache.erase( to_remove );                   // remove from cache
      cache_list.pop_front();                     // remove from list
      // and insert new one
      ce = new cache_elem( b );                   // create new element
      cache[ a ] = ce;
      cache_list.push_back( a );                  // insert in the cache
      ce->listpos = --(cache_list.end());         // update listpos
      ++evictions;
    }

  }

  // Check if element is contained in cache. Return it if it is,
  // otherwise, return NULL.
  //
  cache_elem * contains( const std::string& a ) {
    ci = cache.find( a );
    if ( ci != cache.end() ) {
      return (*ci).second; // pointer to the cache_element
    }
    return NULL;
  }

  // Retrieve string answer from cache. 
  //
  const std::string& get( const std::string& a ) {
    if ( cache_size == 0 ) {
      return empty; // fall through for 0
    }
    ci = cache.find( a );
    if ( ci != cache.end() ) {
      ++hits;
      cache_elem *ce = (*ci).second;
      return ce->ans;
    }
    ++misses;
    return empty;
  }

  // Print some info.
  //
  const std::string stat() {
    std::string ans = to_str(cache.size()) + ":" + "shmer:" + to_str(stores)
      + ":" + to_str(hits)
      + ":" + to_str(misses)
      + ":" + to_str(evictions)
      + ":" + to_str(reinserts);
    return ans;
  }

  void print() {
    cli = cache_list.begin();
    while ( cli != cache_list.end() ) {
      std::cout << "(" << *cli << ")";
      ++cli;
    }
    std::cout << std::endl;
  }
};

#endif
