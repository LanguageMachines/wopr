// $Id$
//

/*****************************************************************************
 * Copyright 2005 - 2011 - 2011 Peter Berck                                         *
 *                                                                           *
 * This file is part of wopr.                                                *
 *                                                                           *
 * wopr is free software; you can redistribute it and/or modify it           *
 * under the terms of the GNU General Public License as published by the     *
 * Free Software Foundation; either version 2 of the License, or (at your    *
 * option) any later version.                                                *
 *                                                                           *
 * wopr is distributed in the hope that it will be useful, but WITHOUT       *
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or     *
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License      *
 * for more details.                                                         *
 *                                                                           *
 * You should have received a copy of the GNU General Public License         *
 * along with wpred; if not, write to the Free Software Foundation,          *
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA               *
 *****************************************************************************/

// ----------------------------------------------------------------------------

/*!
  \file Config.h
  \author Peter Berck
  \date 2004
*/

/*!
  \class Config
  \brief Holds all the information from a config file.
*/

#ifndef _WPREDCONFIG_H
#define _WPREDCONFIG_H

#include <string>
#include <vector>
#include <map>

#include "qlog.h"

// ----------------------------------------------------------------------------
// Class
// ----------------------------------------------------------------------------

class Config {
 private:
  std::string             id;         /*!< Who am I? */
  time_t                  t_start;    /*!< Startup time */
  std::string             fname;      /*!< Name of config file */
  std::string             buf;
  std::map<std::string, std::string> kv;
  int                     status;

public:

  //! Constructor.
  //!
  Config();
  explicit Config( const std::string& );

  //! Destructor.
  //!
  ~Config();

  int  get_status() { return status; }
  void set_status(int s) { status = s; }
  void read_file( const std::string& );
  void process_line( const std::string&, bool );
  void dump_kv();
  std::string kvs_str();
  std::string kvs_str_clean();
  void clear_kv();
  void add_kv( const std::string&, const std::string& );
  void del_kv( const std::string& );
  const std::string& get_value( const std::string& );
  const std::string& get_value( const std::string&, const std::string& );

};

#endif
