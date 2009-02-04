// $Id$

/*****************************************************************************
 * Copyright 2005 - 2009 Peter Berck                                         *
 *                                                                           *
 * This file is part of PETeR.                                               *
 *                                                                           *
 * PETeR is free software; you can redistribute it and/or modify it          *
 * under the terms of the GNU General Public License as published by the     *
 * Free Software Foundation; either version 2 of the License, or (at your    *
 * option) any later version.                                                *
 *                                                                           *
 * PETeR is distributed in the hope that it will be useful, but WITHOUT      *
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or     *
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License      *
 * for more details.                                                         *
 *                                                                           *
 * You should have received a copy of the GNU General Public License         *
 * along with PETeR; if not, write to the Free Software Foundation,          *
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA               *
 *****************************************************************************/

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>

// C includes
//
#include <sys/types.h>
#include <dirent.h>
#include <sys/time.h>
#include <unistd.h>

#include <stdlib.h>  
#include <string.h>  

#include <fcntl.h>
//#include <paths.h>

#include "util.h"

//-----------------------------------------------------------------------------
// Code
//-----------------------------------------------------------------------------

/*!
  Converts a string to an integer (atoi)
*/
int stoi(const std::string& str) {
  int i = atoi( str.c_str() );
  return( i );
}

/*!
  Converts a string to a long.
*/
long stol(const std::string& str) {
  long i = atol( str.c_str() );
  return( i );
}


/*!
  Converts a string to a double.
*/
double stod(const std::string& str) {
  double i = strtod( str.c_str(), NULL );
  return( i );
}


/*!
  Converts seconds to 03w03d02h33m48s
*/
std::string secs_to_str( long t ) {
  const static int div[] = {604800, 86400, 3600, 60, 60};
  const static char *ind[] = { "w\0", "d\0", "h\0", "m\0", "s\0"};

  // if 0 we need this fix.
  //
  if ( t == 0L ) {
    return "00s";
  }

  std::string res_str;
  long rest; // NB fix int/long mixup
  for (int i = 0; i < 4; i++) {
    rest = int(t / div[i]);
    if ( rest > 0 ) { // use >= 0 if you want "00h" etc included.
      t -= rest * div[i];
      res_str += to_str((int)rest, 2) + ind[i];
    }
  }
  if ( t > 0L ) { // just zero seconds becomes nothing.
    res_str += to_str((int)t, 2) + ind[4];
  }
  return res_str;
}

/*!
  Converts seconds to 03w03d02h33m48s, unless larger than
  limit l.
*/
std::string secs_to_str( long t, long l ) {
  if ( t > l ) {
    return ">"+secs_to_str( l );
  }  
  return secs_to_str( t );
}

/*!
  Q&D method to get a string rep. of an integer.
*/
std::string to_str(int i) {
  std::ostringstream ostr;
  ostr << i;
  return ostr.str();
}

/*!
  Q&D method to get a string rep. of an integer, padded to
  length l.
*/
std::string to_str(int i, int l) {
  std::ostringstream ostr;
  ostr << std::setfill('0') << std::setw(l) << i;
  return ostr.str();
}

/*!
  Q&D method to get a string rep. of an integer, padded to
  length l.
*/
std::string to_str(int i, int l, char p) {
  std::ostringstream ostr;
  ostr << std::setfill(p) << std::setw(l) << i;
  return ostr.str();
}

/*!
  Q&D method to get a string rep. of an integer.
*/
std::string to_str(long i) {
  std::ostringstream ostr;
  ostr << i;
  return ostr.str();
}

/*!
  Q&D method to get a string rep. of an integer.
*/
std::string to_str(double i) {
  std::ostringstream ostr;
  ostr << i;
  return ostr.str();
}

/*!
  Q&D method to get a string rep. of an integer.
  see: http://www.arachnoid.com/cpptutor/student3.html
*/
std::string to_str(double i, int p) {
  std::ostringstream ostr;
  //ostr << std::setiosflags(std::ios::fixed) << std::setprecision(p) << i;
  ostr << i;
  return ostr.str();
}

/*!
  Q&D method to get a string rep. of an integer.
*/
std::string to_str(unsigned long i) {
  std::ostringstream ostr;
  ostr << i;
  return ostr.str();
}

/*!
  Q&D method to get a string rep. of a boolean.
*/
std::string to_str(bool b) {
  if ( b == true ) {
    return "true";
  }
  return "false";
}

/*!
  Return today's time in unix seconds.
*/
long now() {
  timeval tv;
  
  gettimeofday(&tv, 0);
  return tv.tv_sec ;
}

/*!
  Return UTC in unix seconds.
*/
time_t utc() {
  return time(0);
}

/*!
  Return today's date as "yyyy/mm/dd hh:mm:ss"
*/
std::string the_date_time() {
  char               timestring[32];
  timeval            tv;
  struct tm          *t;
  
  std::string time_format = std::string("%Y/%m/%d %H:%M:%S");
  gettimeofday(&tv, 0);
  t = localtime(&tv.tv_sec);
  strftime(timestring, 32, time_format.c_str(),  t);
  return( std::string( timestring ) );
}

/*!
  Return epoch seconds s as "yyyy/mm/dd hh:mm:ss"
*/
std::string the_date_time(long s) {
  char               timestring[32];
  struct tm          *t;
  
  std::string time_format = std::string("%Y/%m/%d %H:%M:%S");
  t = localtime(&s);
  strftime(timestring, 32, time_format.c_str(),  t);
  return( std::string( timestring ) );
}

/*!
  Return epoch seconds s as "yyyy/mm/dd hh:mm"
*/
std::string the_date_time_utc(long s) {
  char               timestring[32];
  struct tm          *t;
  
  std::string time_format = std::string("%Y/%m/%d %H:%M");
  t = gmtime(&s);
  strftime(timestring, 32, time_format.c_str(),  t);
  return( std::string( timestring ) );
}

/*!
  Parse "2002/03/28 08:22" fixed fields only.
  putenv("TZ=GMT");?
*/
long parse_date_time(const std::string& d) {
  if ( d.length() == 5 ) { // time only
    int hh = stoi( d.substr(0, 2) );
    int mm = stoi( d.substr(3, 2) );
    long s = hh * 3600L + mm * 60L;
    return s;
  }
  if ( d.length() < 16 ) {
    return  0L ;
  }
  int yy = stoi( d.substr(0, 4) );
  int mm = stoi( d.substr(5, 2) );
  int dd = stoi( d.substr(8, 2) );
  int hh = stoi( d.substr(11, 2) );
  int mn = stoi( d.substr(14, 2) );
  
  struct tm t_s;
  extern time_t timezone;
  extern int daylight;

  t_s.tm_sec  = 0;//timezone; /* seconds after the minute [0, 61] */
  t_s.tm_min  = mn;       /* minutes after the hour [0, 59] */
  t_s.tm_hour = hh;       /* hour since midnight [0, 23] */
  t_s.tm_mday = dd;       /* day of the month [1, 31] */
  t_s.tm_mon  = mm - 1;   /* months since January [0, 11] */
  t_s.tm_year = yy - 1900;/* years since 1900 */
  t_s.tm_isdst = 0;//daylight;

  long s = mktime( &t_s );
  return s;
}

void set_flag(unsigned long& var, unsigned long& f) {
  var |= f;
}

void clear_flag(unsigned long& var, unsigned long& f) {
  var &= ~f;
}

bool is_set(unsigned long& var, unsigned long& f) {
  return var & f;
}


/*!
  \brief Delay so many u-secs.
*/
void delay(long delay) {
  timeval tv;

  tv.tv_usec = delay % 1000000L;
  tv.tv_sec  = delay / 1000000L;

  select( 0, NULL ,NULL ,NULL, &tv );
}

/*!
  \brief Delay so many m-secs.
*/
void udelay(long delay) {
  timeval tv;

  tv.tv_usec = delay;
  tv.tv_sec  = 0L;

  select( 0, NULL ,NULL ,NULL, &tv );
}

void arg_split(char *buf, char **args) {
  bool in_string = 0;

  while ( *buf != '\0' ) {

    while ( (*buf == ' ') || (*buf == '\t') ) {
      if ( ! in_string ) {
	*buf++ = '\0';
      }
    }
    
    if ( ! in_string ) { 
      *args++ = buf;
    }

    while ( in_string || ((*buf != '\0') && (*buf != ' ') && (*buf != '\t'))) {
      if ( *buf == '"' ) {
	in_string = ( ! in_string );
      }
      buf++;
    }

  }
  *args = NULL;
}

bool check_dir( const std::string& name ) {
  DIR *tmp = opendir( name.c_str() );
  if ( tmp != NULL ) {
    closedir(tmp);
    return true;
  }
  return false;
}

/*
std::stringstream s ( "This is a test" );
std::string token;

while ( s>> token )
  std::cout<< token <<'\n';

// Clear the stream state
s.clear();
// Rewind the stream
s.seekg ( 0, std::ios::beg );

while ( s>> token )
  std::cout<< token <<'\n';

    std::stringstream foo( a_line ); 
    std::string a_word;
    while ( foo >> a_word ) {
      l.log( a_word );
    }
*/
void Tokenize(const std::string& buffer, std::vector<std::string>& tokens ) {
  std::stringstream foo( buffer ); 
  std::string a_word;
  while ( foo >> a_word ) {
    tokens.push_back( a_word );
  }
}

void Tokenize(const std::string& buffer, std::vector<std::string>& tokens,
              const char delimiter) {
  int pos = 0, pos_ant = 0;
  
  pos = buffer.find(delimiter, pos_ant);
  while ( pos != std::string::npos ) {
    std::string token = buffer.substr(pos_ant, pos-pos_ant);
    tokens.push_back(token);
    pos_ant = pos+1;
    pos = buffer.find(delimiter, pos_ant);
  }

  if ( ! buffer.empty() ) {
    std::string token = buffer.substr(pos_ant, buffer.size() );
    tokens.push_back( token );
  }
}

std::string status_to_str(int status) {
  int sig = 0;
  int res = 0;

  if ( WIFSIGNALED(status) ) {
    sig = WTERMSIG(status); //status & 127;
  }
  res = WEXITSTATUS(status); //result code
  
  return to_str(res)+"/"+to_str(sig);
}

std::string trim(std::string const& source, char const* delims ) {
  std::string result( source );
  std::string::size_type index = result.find_last_not_of(delims);
  if ( index != std::string::npos ) {
    result.erase( ++index );
  }
  
  index = result.find_first_not_of( delims );
  if ( index != std::string::npos ) {
    result.erase( 0, index );
  } else {
    result.erase();
  }
  
  return result;
}

bool is_numeric( std::string s ) {
  for ( int i = 0; i < s.length(); i++ ) {
    char c = s.at(i);
    if ( isdigit(c) || (c == '.') || (c == ',') || (c == '+') || 
	 (c == '-') ) {
      continue;// make it: c = 32, then fall through rest?
    }
    return false;
  }
}

//-----------------------------------------------------------------------------
