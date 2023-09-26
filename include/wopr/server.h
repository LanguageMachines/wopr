// $Id$
//

/*****************************************************************************
 * Copyright 2007-2011 Peter Berck                                           *
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
 * along with wopr; if not, write to the Free Software Foundation,           *
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA               *
 *****************************************************************************/

#ifndef _SERVER_H
#define _SERVER_H

#include <string>
#include <vector>

#include "qlog.h"
#include "Config.h"

#ifdef TIMBL
#include "timbl/TimblAPI.h"
#endif

int server2( Logfile&, Config& );
std::string json_safe( const std::string& );
std::string str_clean( const std::string& );

#ifdef TIMBL
int socket_file( Logfile&, Config&,
		 Timbl::TimblAPI *, const std::string&,
		 std::map<std::string,int>&, unsigned long);
int parse_distribution( const Timbl::ClassDistribution*,
			std::map<std::string, double>&);
int dist_to_xml( const Timbl::ClassDistribution*, std::string&, const std::string& );
#endif

int server3( Logfile&, Config& );
int server4( Logfile&, Config& );
int xmlserver( Logfile&, Config& );
int mbmt( Logfile&, Config& );
int webdemo( Logfile&, Config& );
int one( Logfile&, Config& );
int hapax_vector( std::vector<std::string>&, const std::map<std::string,int>& );
int vector_to_string( const std::vector<std::string>& , std::string&  );
int server_mg( Logfile&, Config& );
#endif
