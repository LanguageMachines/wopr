// $Id$

/*****************************************************************************
 * Copyright 2008 - 2011-2009 Peter Berck                                           *
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
  \author Peter Berck
  \date 2009
*/

#ifndef _ELEMENTS_H
#define _ELEMENTS_H

#include <vector>
#include <string>

// ----------------------------------------------------------------------------
// Classes
// ----------------------------------------------------------------------------

class distr_elem {
 public:
  std::string name; 
  double      freq;
  double      lexfreq;

  distr_elem();
  ~distr_elem();

  distr_elem( const std::string& s, double d1, double d2);

  //distr_elem(const distr_elem&);
};

class distr_elem_cmp_ptr {
 public:
  bool operator() (const distr_elem* de1, const distr_elem* de2) const {
    return de1->freq < de2->freq;
  } 
}; 

#endif
