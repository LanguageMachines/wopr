// $Id: wex.h 1826 2007-12-05 10:16:34Z pberck $

/*****************************************************************************
 * Copyright 2009 Peter Berck                                                *
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

// ----------------------------------------------------------------------------

#ifndef _wex_h
#define _wex_h

#include <string>
#include <vector>

#include "Config.h"

// ----------------------------------------------------------------------------

int multi( Logfile&, Config& );
int read_kv_from_file(std::ifstream&, std::map<std::string, std::string>&);
int read_classifiers_from_file( std::ifstream& file,std::vector<Classifier*>& );
int multi_dist( Logfile&, Config& );

#endif
