// $Id$

/*****************************************************************************
 * Copyright 2010 - 2011 Peter Berck                                                *
 *                                                                           *
 * This file is part of Wopr.                                                *
 *                                                                           *
 * Wopr is free software; you can redistribute it and/or modify it           *
 * under the terms of the GNU General Public License as published by the     *
 * Free Software Foundation; either version 2 of the License, or (at your    *
 * option) any later version.                                                *
 *                                                                           *
 * Wopr is distributed in the hope that it will be useful, but WITHOUT       *
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or     *
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License      *
 * for more details.                                                         *
 *                                                                           *
 * You should have received a copy of the GNU General Public License         *
 * along with Wopr; if not, write to the Free Software Foundation,           *
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA               *
 *****************************************************************************/

// ----------------------------------------------------------------------------

#ifndef _ngram_elements_h
#define _ngram_elements_h

struct ngl_elem {
  long freq;
  int  n;
};

struct ngram_elem {
  double p;
  double l10p;
  int    n;
  std::string ngram;
};

struct ngde { // ngram dist element
  std::string token;
  int freq;
  double prob;
  double l10prob;
  bool operator<(const ngde& rhs) const {
    // fall back to prob if freqs are equal?
    return freq > rhs.freq; // || prob > rhs.prob;
  }
};
struct ngmd { // ngram meta data
  double distr_count;
  double distr_sum;
  std::vector<ngde> distr;
};

struct ngp { // contains the prob, l10prob, ...
  double prob;
  double l10prob;
};

#endif
