/*****************************************************************************
 * Copyright 2008 - 2016 Peter Berck                                         *
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

#ifndef _PREDEDIT_H
#define _PREDEDIT_H

#include "Context.h"

struct t_distr_elem;

void generate_next( Timbl::TimblAPI*, const std::string&, std::vector<t_distr_elem>& );
void generate_tree( Timbl::TimblAPI*, Context&, std::vector<std::string>&, int, std::vector<int>&,int, const std::string& );
int explode( const std::string&, std::vector<std::string>&);
void window_word_letters( const std::string&, const std::string&, int, Context&, std::vector<std::string>&);
void window_words_letters( const std::string&, int, Context&, std::vector<std::string>&);
int pdt( Logfile&, Config& );
int pdt2( Logfile&, Config& );
//void generate_next(  Timbl::TimblAPI*, Config&, std::string, int, std::vector<t_distr_elem>& );
size_t count_keys( const std::string&);
int window_letters( Logfile&, Config& );
int pdt2web( Logfile&, Config& );

#endif
